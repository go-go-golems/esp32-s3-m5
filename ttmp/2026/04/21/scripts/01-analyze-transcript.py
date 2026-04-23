#!/usr/bin/env python3
"""
Analyze a Pi agent transcript JSONL file to understand
hardware research methodology and documentation patterns.

This script was used to analyze the M5Stack Tab5 research session.
It extracts tool usage, documentation patterns, research phases,
and generates structured reports.

Usage:
    python3 01-analyze-transcript.py <path-to.jsonl> [--output-dir <dir>]
"""

import json
import sys
import argparse
from collections import Counter, defaultdict
from pathlib import Path
from datetime import datetime


def load_transcript(path: str) -> list[dict]:
    """Load all messages from a Pi agent JSONL transcript."""
    messages = []
    with open(path, 'r') as f:
        for line in f:
            if line.strip():
                messages.append(json.loads(line.strip()))
    return messages


def extract_tool_calls(messages: list[dict]) -> list[dict]:
    """
    Extract all tool calls from the transcript.
    
    Pi agent transcript format:
    - message.role = 'assistant' with content items of type 'toolCall'
    - Each toolCall has: id, name, arguments
    """
    tool_calls = []
    for msg in messages:
        if msg.get('type') == 'message':
            msg_data = msg.get('message', {})
            role = msg_data.get('role', '')
            content = msg_data.get('content', [])
            
            if role == 'assistant' and isinstance(content, list):
                for item in content:
                    if item.get('type') == 'toolCall':
                        args = item.get('arguments', {})
                        if isinstance(args, str):
                            try:
                                args = json.loads(args)
                            except:
                                args = {'raw': args}
                        
                        tool_calls.append({
                            'id': item.get('id'),
                            'name': item.get('name'),
                            'arguments': args,
                            'timestamp': msg.get('timestamp'),
                        })
    
    return tool_calls


def extract_user_prompts(messages: list[dict]) -> list[dict]:
    """Extract all user prompts from the transcript."""
    prompts = []
    for msg in messages:
        if msg.get('type') == 'message':
            msg_data = msg.get('message', {})
            role = msg_data.get('role', '')
            content = msg_data.get('content', [])
            
            if role == 'user' and isinstance(content, list):
                for item in content:
                    if item.get('type') == 'text':
                        prompts.append({
                            'id': msg.get('id'),
                            'timestamp': msg.get('timestamp'),
                            'content': item.get('text', ''),
                        })
    
    return prompts


def categorize_tool_calls(tool_calls: list[dict]) -> dict:
    """Categorize tool calls by type and extract relevant details."""
    categories = {
        'bash': [],
        'read': [],
        'write': [],
        'edit': [],
        'web_search': [],
        'understand_image': [],
        'playwright': [],
        'other': [],
    }
    
    for tc in tool_calls:
        name = tc.get('name', 'unknown')
        args = tc.get('arguments', {})
        
        if name == 'bash':
            cmd = args.get('command', '') if isinstance(args, dict) else ''
            categories['bash'].append({
                'command': cmd[:200],
                'full_command': cmd,
            })
        elif name == 'read':
            path = args.get('path', '') if isinstance(args, dict) else ''
            categories['read'].append({
                'path': path,
            })
        elif name == 'write':
            path = args.get('path', '') if isinstance(args, dict) else ''
            content = args.get('content', '')[:100] if isinstance(args, dict) else ''
            categories['write'].append({
                'path': path,
                'content_preview': content,
            })
        elif name == 'web_search':
            query = args.get('query', '') if isinstance(args, dict) else ''
            categories['web_search'].append({
                'query': query,
            })
        elif name.startswith('playwright'):
            categories['playwright'].append({
                'name': name,
                'args': str(args)[:100] if isinstance(args, dict) else str(args),
            })
        else:
            categories['other'].append({'name': name})
    
    return categories


def analyze_documentation_workflow(categorized: dict) -> dict:
    """Analyze how documentation tools (docmgr, etc.) were used."""
    docmgr_commands = []
    docmgr_operations = Counter()
    write_operations = []
    
    # Analyze bash commands for docmgr usage
    for cmd_info in categorized.get('bash', []):
        cmd = cmd_info.get('full_command', '')
        if 'docmgr' in cmd.lower():
            # Extract docmgr subcommand
            parts = cmd.split('docmgr')
            if len(parts) > 1:
                subcmd = parts[1].split()[0] if parts[1].strip() else 'unknown'
                docmgr_operations[subcmd] += 1
            docmgr_commands.append(cmd[:200])
        
        if 'defuddle' in cmd.lower():
            docmgr_commands.append(f'[defuddle] {cmd[:200]}')
        
        if 'remark' in cmd.lower():
            docmgr_commands.append(f'[remarkable] {cmd[:200]}')
    
    # Analyze write operations for documentation output
    for write_info in categorized.get('write', []):
        path = write_info.get('path', '')
        if 'ttmp' in path or 'docmgr' in path.lower() or 'obsidian' in path.lower():
            write_operations.append(path)
    
    return {
        'docmgr_commands': docmgr_commands,
        'docmgr_operations': dict(docmgr_operations),
        'doc_write_operations': write_operations,
    }


def analyze_research_phases(prompts: list[dict], categorized: dict) -> list[dict]:
    """Identify research phases based on user prompts."""
    phases = []
    current_phase = None
    phase_number = 0
    
    phase_keywords = {
        'initialization': ['clone', 'download', 'datasheet', 'datasheets', 'pinout'],
        'firmware': ['firmware', 'build', 'flash', 'idf.py'],
        'documentation': ['docmgr', 'ticket', 'design', 'guide', 'documentation'],
        'troubleshooting': ['bug', 'error', 'panic', 'crash', 'fix', 'issue'],
        'testing': ['test', 'verify', 'monitor', 'logs'],
    }
    
    for prompt in prompts:
        content = prompt.get('content', '')
        
        # Detect phase transitions
        detected_phase = None
        for phase_name, keywords in phase_keywords.items():
            if any(kw.lower() in content.lower() for kw in keywords):
                detected_phase = phase_name
                break
        
        if detected_phase and detected_phase != current_phase:
            if current_phase:
                phases.append({
                    'phase': current_phase,
                    'end_prompt': prompt.get('id'),
                })
            current_phase = detected_phase
            phase_number += 1
            phases.append({
                'phase': current_phase,
                'start_prompt': prompt.get('id'),
                'number': phase_number,
            })
    
    return phases


def generate_tool_summary(categorized: dict) -> dict:
    """Generate a summary of tool usage patterns."""
    summary = {}
    
    # Tool counts
    tool_counts = Counter()
    for name in categorized.keys():
        if name != 'other':
            tool_counts[name] = len(categorized[name])
    
    summary['tool_counts'] = dict(tool_counts.most_common())
    
    # Most common bash commands
    bash_cmds = [tc.get('command', '')[:80] for tc in categorized.get('bash', [])]
    bash_counts = Counter(bash_cmds)
    summary['top_bash_commands'] = dict(bash_counts.most_common(15))
    
    # File types read
    file_types = Counter()
    for read_info in categorized.get('read', []):
        path = read_info.get('path', '')
        if path.endswith('.pdf'):
            file_types['PDF'] += 1
        elif path.endswith('.md'):
            file_types['Markdown'] += 1
        elif path.endswith('.c') or path.endswith('.h'):
            file_types['C/C++'] += 1
        elif path.endswith('.py'):
            file_types['Python'] += 1
        elif path.endswith('.json'):
            file_types['JSON'] += 1
        elif 'datasheet' in path.lower():
            file_types['Datasheet'] += 1
        elif 'ttmp' in path or 'docmgr' in path.lower():
            file_types['Docmgr'] += 1
        else:
            file_types['Other'] += 1
    
    summary['file_types_read'] = dict(file_types.most_common())
    
    return summary


def generate_report(
    messages: list[dict],
    tool_calls: list[dict],
    prompts: list[dict],
    categorized: dict,
    doc_workflow: dict,
    phases: list[dict],
    tool_summary: dict,
) -> str:
    """Generate a comprehensive analysis report."""
    
    report = f"""# Hardware Research Session Analysis

## Session Overview

- **Total messages**: {len(messages)}
- **Total tool calls**: {len(tool_calls)}
- **Total user prompts**: {len(prompts)}

## Tool Usage Summary

### Tool Counts
"""
    
    for tool, count in tool_summary.get('tool_counts', {}).items():
        report += f"- **{tool}**: {count} calls\n"
    
    report += """
### Top Bash Commands
"""
    for cmd, count in tool_summary.get('top_bash_commands', {}).items():
        report += f"- {count}x: `{' '.join(cmd.split()[:6])}...`\n"
    
    report += """
### File Types Read
"""
    for ftype, count in tool_summary.get('file_types_read', {}).items():
        report += f"- {ftype}: {count}\n"
    
    report += """
## Documentation Workflow

### Docmgr Operations
"""
    for op, count in doc_workflow.get('docmgr_operations', {}).items():
        report += f"- `docmgr {op}`: {count} times\n"
    
    report += """
### Documentation Write Operations
"""
    for path in doc_workflow.get('doc_write_operations', [])[:20]:
        report += f"- `{path}`\n"
    
    report += """
### Docmgr Command Examples
"""
    for cmd in doc_workflow.get('docmgr_commands', [])[:10]:
        if '[defuddle]' in cmd or '[remarkable]' in cmd:
            report += f"- {cmd}\n"
        else:
            report += f"- `{cmd[:120]}`\n"
    
    report += """
## Research Phases

"""
    phase_num = 0
    for phase_info in phases:
        if 'number' in phase_info:
            phase_num = phase_info['number']
            report += f"### Phase {phase_num}: {phase_info['phase'].title()}\n"
            report += f"- Started at prompt: `{phase_info.get('start_prompt', 'unknown')}`\n\n"
        elif 'end_prompt' in phase_info:
            report += f"- Ended at prompt: `{phase_info.get('end_prompt', 'unknown')}`\n\n"
    
    report += """
## User Prompt Sequence

"""
    for i, prompt in enumerate(prompts[:20]):
        content = prompt.get('content', '')[:150].replace('\n', ' ')
        report += f"{i+1}. {content}...\n"
    
    report += """
---

*Generated by analyze-transcript.py*
"""
    
    return report


def main():
    parser = argparse.ArgumentParser(description='Analyze Pi agent transcript')
    parser.add_argument('input', help='Path to JSONL transcript file')
    parser.add_argument('--output-dir', '-o', default='.', help='Output directory')
    args = parser.parse_args()
    
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Loading transcript from {input_path}...")
    messages = load_transcript(str(input_path))
    print(f"Loaded {len(messages)} messages")
    
    print("Extracting tool calls...")
    tool_calls = extract_tool_calls(messages)
    print(f"Found {len(tool_calls)} tool calls")
    
    print("Extracting user prompts...")
    prompts = extract_user_prompts(messages)
    print(f"Found {len(prompts)} user prompts")
    
    print("Categorizing tool calls...")
    categorized = categorize_tool_calls(tool_calls)
    
    print("Analyzing documentation workflow...")
    doc_workflow = analyze_documentation_workflow(categorized)
    
    print("Identifying research phases...")
    phases = analyze_research_phases(prompts, categorized)
    
    print("Generating tool summary...")
    tool_summary = generate_tool_summary(categorized)
    
    print("Generating report...")
    report = generate_report(
        messages, tool_calls, prompts, categorized,
        doc_workflow, phases, tool_summary
    )
    
    # Save report
    report_path = output_dir / 'analysis-report.md'
    report_path.write_text(report)
    print(f"Report saved to {report_path}")
    
    # Save structured data as JSON
    data = {
        'tool_counts': tool_summary.get('tool_counts', {}),
        'file_types_read': tool_summary.get('file_types_read', {}),
        'docmgr_operations': doc_workflow.get('docmgr_operations', {}),
        'phases': phases,
        'prompts': [
            {'id': p.get('id'), 'content': p.get('content')[:300]}
            for p in prompts
        ],
    }
    
    data_path = output_dir / 'analysis-data.json'
    with open(data_path, 'w') as f:
        json.dump(data, f, indent=2)
    print(f"Structured data saved to {data_path}")


if __name__ == '__main__':
    main()