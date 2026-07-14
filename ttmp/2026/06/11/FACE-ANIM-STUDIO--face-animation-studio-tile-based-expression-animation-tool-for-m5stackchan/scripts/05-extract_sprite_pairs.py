#!/usr/bin/env python3
"""
extract_sprite_pairs.py — Extract aligned sprite pairs from matching grid sheets.

Uses weighted cross-correlation on row-sum profiles to find the optimal
vertical offset that aligns paired sprites. This handles cases where
different variants (e.g., white vs red) have different content extents
(like extra features above the main body).

Algorithm:
  1. Crop grid cells at identical positions from both sheets
  2. Scale both to target width (135px for M5StackChan)
  3. Compute binary masks from both scaled tiles
  4. Find optimal vertical offset via weighted cross-correlation:
     - Row-sum profiles capture the horizontal extent at each row
     - Weight by minimum of both profiles (emphasizes common wide parts)
     - Scan offsets from -max_offset to +max_offset
  5. Place both tiles on 135×240 canvas with calculated offsets

Usage:
  python3 extract_sprite_pairs.py <sheet_a> <sheet_b> [options]

Options:
  --output-dir DIR    Output directory (default: assets/tiles_clock)
  --cols COLS         Grid columns (default: 4)
  --rows ROWS         Grid rows (default: 4)
  --width WIDTH       Target width in pixels (default: 135)
  --height HEIGHT     Target canvas height (default: 240)
  --max-offset N      Maximum offset to search (default: 40)
  --variant-a NAME    Name for sheet A (default: white)
  --variant-b NAME    Name for sheet B (default: red)
  --threshold PCT     Black threshold percentage (default: 2)

Requirements:
  - ImageMagick (convert, identify)
  - Python 3.7+ with numpy
"""

import argparse
import os
import subprocess
import sys
import tempfile

import numpy as np


def run_im(args, **kwargs):
    """Run ImageMagick command."""
    result = subprocess.run(args, capture_output=True, text=True, **kwargs)
    if result.returncode != 0:
        print(f"ImageMagick error: {result.stderr}", file=sys.stderr)
    return result


def get_binary_mask(path):
    """Load image as binary numpy mask (1=content, 0=black)."""
    r = subprocess.run(
        ['convert', path, '-colorspace', 'Gray', '-threshold', '2%',
         '-negate', '-depth', '8', 'gray:-'],
        capture_output=True
    )
    w = int(run_im(['identify', '-format', '%w', path]).stdout)
    h = int(run_im(['identify', '-format', '%h', path]).stdout)
    arr = np.frombuffer(r.stdout, dtype=np.uint8).reshape(h, w)
    return (arr > 0).astype(float)


def find_alignment_offset(mask_a, mask_b, max_offset=40):
    """
    Find the vertical offset that best aligns mask_b to mask_a.
    
    Uses weighted cross-correlation of row-sum profiles:
    - Row sums capture horizontal extent (wider = more important)
    - Weight by minimum of both profiles at each row
    - This emphasizes the common wide parts (e.g., clock body)
    - And de-emphasizes narrow features (e.g., raised eyebrows)
    
    Returns: (offset, correlation) where positive offset shifts B down.
    """
    profile_a = (mask_a > 0).sum(axis=1)  # row sums
    profile_b = (mask_b > 0).sum(axis=1)
    
    best_offset = 0
    best_corr = -1.0
    
    for off in range(-max_offset, max_offset + 1):
        if off >= 0:
            p1 = profile_a[off:]
            p2 = profile_b[:len(profile_a) - off]
        else:
            p1 = profile_a[:len(profile_a) + off]
            p2 = profile_b[-off:]
        
        if len(p1) < 30:
            continue
        
        # Weight by minimum of both profiles (emphasizes common wide parts)
        weights = np.minimum(p1, p2)
        if weights.sum() < 100:
            continue
        
        # Weighted correlation
        w1 = p1 * weights
        w2 = p2 * weights
        
        corr = np.corrcoef(w1, w2)[0, 1]
        if corr > best_corr:
            best_corr = corr
            best_offset = off
    
    return best_offset, best_corr


def extract_pairs(sheet_a, sheet_b, output_dir='assets/tiles_clock',
                  cols=4, rows=4, width=135, height=240,
                  max_offset=40, variant_a='white', variant_b='red',
                  threshold=2):
    """Main extraction pipeline."""
    
    total = cols * rows
    tmpdir = tempfile.mkdtemp(prefix='sprite_pairs_')
    
    print(f"=== Sprite Pair Extractor (weighted cross-correlation) ===")
    print(f"Variant A ({variant_a}): {sheet_a}")
    print(f"Variant B ({variant_b}): {sheet_b}")
    print(f"Grid: {cols}x{rows}, Target: {width}x{height}")
    
    # Step 1: Crop grid cells
    print(f"\nStep 1: Cropping {cols}x{rows} grid cells...")
    run_im(['convert', sheet_a, '-crop', f'{cols}x{rows}@', '+repage',
            os.path.join(tmpdir, f'{variant_a}_%02d.png')])
    run_im(['convert', sheet_b, '-crop', f'{cols}x{rows}@', '+repage',
            os.path.join(tmpdir, f'{variant_b}_%02d.png')])
    
    # Step 2: Scale to target width
    print(f"Step 2: Scaling to {width}px wide...")
    os.makedirs(output_dir, exist_ok=True)
    
    for idx in range(total):
        idx_padded = f'{idx:02d}'
        for variant in [variant_a, variant_b]:
            src = os.path.join(tmpdir, f'{variant}_{idx_padded}.png')
            dst = os.path.join(tmpdir, f'scaled_{variant}_{idx_padded}.png')
            run_im(['convert', src, '-resize', f'{width}x', dst])
    
    # Step 3: Align and place on canvas
    print(f"Step 3: Computing alignment offsets...")
    
    for idx in range(total):
        idx_padded = f'{idx:02d}'
        
        scaled_a = os.path.join(tmpdir, f'scaled_{variant_a}_{idx_padded}.png')
        scaled_b = os.path.join(tmpdir, f'scaled_{variant_b}_{idx_padded}.png')
        
        mask_a = get_binary_mask(scaled_a)
        mask_b = get_binary_mask(scaled_b)
        
        offset, corr = find_alignment_offset(mask_a, mask_b, max_offset)
        
        # Calculate placement positions
        ha = mask_a.shape[0]
        hb = mask_b.shape[0]
        
        top_a = height - ha  # bottom-align variant A
        top_b = height - hb + offset  # apply offset to variant B
        
        # Clamp to canvas
        top_a = max(0, top_a)
        top_b = max(0, top_b)
        
        # Place on canvas
        for variant, top in [(variant_a, top_a), (variant_b, top_b)]:
            src = os.path.join(tmpdir, f'scaled_{variant}_{idx_padded}.png')
            dst = os.path.join(output_dir, f'clock_{variant}_{idx_padded}.png')
            run_im([
                'convert', '-size', f'{width}x{height}', 'xc:black',
                src, '-gravity', 'north', '-geometry', f'+0+{top}',
                '-compose', 'over', '-composite',
                '-black-threshold', f'{threshold}%',
                dst
            ])
        
        print(f"  Pair {idx_padded}: offset={offset:+d} corr={corr:.3f} "
              f"{variant_a}_top={top_a} {variant_b}_top={top_b}")
    
    # Cleanup
    import shutil
    shutil.rmtree(tmpdir, ignore_errors=True)
    
    print(f"\nDone! {total} pairs written to {output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description='Extract aligned sprite pairs from matching grid sheets')
    parser.add_argument('sheet_a', help='First variant sprite sheet')
    parser.add_argument('sheet_b', help='Second variant sprite sheet')
    parser.add_argument('--output-dir', default='assets/tiles_clock',
                        help='Output directory')
    parser.add_argument('--cols', type=int, default=4, help='Grid columns')
    parser.add_argument('--rows', type=int, default=4, help='Grid rows')
    parser.add_argument('--width', type=int, default=135, help='Target width')
    parser.add_argument('--height', type=int, default=240, help='Canvas height')
    parser.add_argument('--max-offset', type=int, default=40,
                        help='Maximum alignment offset to search')
    parser.add_argument('--variant-a', default='white', help='Name for sheet A')
    parser.add_argument('--variant-b', default='red', help='Name for sheet B')
    parser.add_argument('--threshold', type=int, default=2,
                        help='Black threshold percentage')
    
    args = parser.parse_args()
    
    extract_pairs(
        args.sheet_a, args.sheet_b,
        output_dir=args.output_dir,
        cols=args.cols, rows=args.rows,
        width=args.width, height=args.height,
        max_offset=args.max_offset,
        variant_a=args.variant_a, variant_b=args.variant_b,
        threshold=args.threshold
    )


if __name__ == '__main__':
    main()
