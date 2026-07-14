#!/usr/bin/env python3
"""
Normalize face tiles: auto-crop black borders, scale to uniform face height,
bottom-align, and output on a consistent canvas.

Usage:
    python3 normalize_tiles.py assets/tiles/ assets/tiles_normalized/
"""

import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Pillow required: pip install Pillow")
    sys.exit(1)

# Target face height in pixels (all faces will be scaled to this height)
# This determines the final resolution of the face content.
TARGET_FACE_HEIGHT = 256

# Final canvas size — must be tall enough for the tallest face + some padding
# and wide enough for the widest face. We'll compute it dynamically but set
# a minimum.
CANVAS_WIDTH = 280
CANVAS_HEIGHT = 340


def find_content_bbox(img):
    """Find the bounding box of non-black content in the image.
    Returns (left, top, right, bottom) or None if image is all black."""
    # Convert to grayscale for analysis
    gray = img.convert('L')
    
    # Threshold — pixels above this are considered "content"
    threshold = 10
    
    pixels = gray.load()
    width, height = gray.size
    
    left = width
    top = height
    right = 0
    bottom = 0
    
    for y in range(height):
        for x in range(width):
            if pixels[x, y] > threshold:
                if x < left: left = x
                if x > right: right = x
                if y < top: top = y
                if y > bottom: bottom = y
    
    if right == 0 and bottom == 0:
        return None  # All black
    
    # Add 1 to make it exclusive (like PIL's bbox format)
    return (left, top, right + 1, bottom + 1)


def process_tile(src_path, dst_path, target_face_height, canvas_w, canvas_h):
    """Process a single tile: crop, scale, bottom-align, pad to canvas."""
    img = Image.open(src_path)
    
    # Step 1: Auto-crop black borders
    bbox = find_content_bbox(img)
    if bbox is None:
        # All black — create a blank canvas
        result = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 255))
        result.save(dst_path)
        return {'face_w': 0, 'face_h': 0, 'scale': 1.0}
    
    cropped = img.crop(bbox)
    face_w, face_h = cropped.size
    
    # Step 2: Scale to target face height
    scale = target_face_height / face_h
    new_w = int(face_w * scale)
    new_h = target_face_height  # Exactly target height
    
    scaled = cropped.resize((new_w, new_h), Image.LANCZOS)
    
    # Step 3: Bottom-align on canvas
    result = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 255))
    
    # Center horizontally, align to bottom
    x = (canvas_w - new_w) // 2
    y = canvas_h - new_h  # Bottom-aligned
    
    result.paste(scaled, (x, y))
    
    # Save
    dst_path = Path(dst_path)
    dst_path.parent.mkdir(parents=True, exist_ok=True)
    result.save(str(dst_path))
    
    return {'face_w': new_w, 'face_h': new_h, 'scale': scale, 'bbox': bbox}


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input_dir> <output_dir>")
        sys.exit(1)
    
    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    
    if not input_dir.exists():
        print(f"Input directory not found: {input_dir}")
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Phase 1: Analyze all tiles to find the largest face dimensions
    # so we can size the canvas appropriately
    print("Phase 1: Analyzing tiles...")
    tiles = sorted(input_dir.glob("*.png"))
    
    if not tiles:
        print("No PNG files found in input directory")
        sys.exit(1)
    
    # First pass: auto-crop and measure original face sizes
    face_sizes = []
    for tile_path in tiles:
        img = Image.open(tile_path)
        bbox = find_content_bbox(img)
        if bbox:
            face_w = bbox[2] - bbox[0]
            face_h = bbox[3] - bbox[1]
            face_sizes.append((tile_path.name, face_w, face_h, bbox))
        else:
            face_sizes.append((tile_path.name, 0, 0, None))
    
    # Compute scale based on the median face height (more robust than mean)
    heights = [h for _, _, h, _ in face_sizes if h > 0]
    if not heights:
        print("No content found in any tile!")
        sys.exit(1)
    
    heights.sort()
    median_height = heights[len(heights) // 2]
    print(f"  Median face height: {median_height}px")
    print(f"  Min: {heights[0]}, Max: {heights[-1]}")
    
    # Compute what the scaled face dimensions will be
    scale = TARGET_FACE_HEIGHT / median_height
    max_scaled_w = 0
    max_scaled_h = 0
    
    for name, fw, fh, bbox in face_sizes:
        if fh > 0:
            sw = int(fw * TARGET_FACE_HEIGHT / fh)
            sh = TARGET_FACE_HEIGHT
            max_scaled_w = max(max_scaled_w, sw)
            max_scaled_h = max(max_scaled_h, sh)
    
    # Set canvas size with some padding
    canvas_w = max(CANVAS_WIDTH, max_scaled_w + 20)
    canvas_h = max(CANVAS_HEIGHT, max_scaled_h + 20)
    
    print(f"  Target face height: {TARGET_FACE_HEIGHT}px")
    print(f"  Scale factor: {scale:.3f}")
    print(f"  Canvas size: {canvas_w}×{canvas_h}")
    
    # Phase 2: Process all tiles
    print("\nPhase 2: Processing tiles...")
    results = {}
    for tile_path in tiles:
        result = process_tile(
            str(tile_path),
            str(output_dir / tile_path.name),
            TARGET_FACE_HEIGHT,
            canvas_w,
            canvas_h
        )
        results[tile_path.name] = result
        print(f"  {tile_path.name}: bbox={result.get('bbox')} → "
              f"face {result['face_w']}×{result['face_h']} "
              f"(scale {result['scale']:.3f})")
    
    # Phase 3: Verify alignment — check that all bottom edges are at the same y
    print(f"\nPhase 3: Verification")
    print(f"  All faces bottom-aligned at y={canvas_h - TARGET_FACE_HEIGHT}")
    print(f"  Canvas: {canvas_w}×{canvas_h}")
    print(f"  All faces scaled to height={TARGET_FACE_HEIGHT}px")
    print(f"  Processed {len(tiles)} tiles → {output_dir}")
    
    # Also create a contact sheet for visual verification
    contact_w = canvas_w * 4 + 30
    contact_h_per_sheet = canvas_h * 4 + 30
    contact = Image.new('RGBA', (contact_w, contact_h_per_sheet * 3 + 60), (30, 30, 30, 255))
    
    for sheet_num in range(1, 4):
        for idx in range(16):
            name = f"sheet{sheet_num}_{idx:02d}.png"
            tile_path = output_dir / name
            if tile_path.exists():
                tile_img = Image.open(tile_path)
                col = idx % 4
                row = idx // 4
                x = col * (canvas_w + 10) + 10
                y = (sheet_num - 1) * contact_h_per_sheet + row * (canvas_h + 10) + 10
                contact.paste(tile_img, (x, y))
    
    contact_path = output_dir / "contact_sheet.png"
    contact.save(str(contact_path))
    print(f"  Contact sheet: {contact_path}")


if __name__ == '__main__':
    main()
