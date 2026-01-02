#!/usr/bin/env python3
"""
Populate SQLite database with git commit history and statistics.
"""
import sqlite3
import subprocess
import re
from datetime import datetime

# Create database
conn = sqlite3.connect('project_history.db')
cursor = conn.cursor()

# Create tables
cursor.execute('''
CREATE TABLE IF NOT EXISTS commits (
    hash TEXT PRIMARY KEY,
    author_name TEXT,
    author_email TEXT,
    date TEXT,
    message TEXT,
    parent_hash TEXT,
    files_changed INTEGER,
    insertions INTEGER,
    deletions INTEGER
)
''')

cursor.execute('''
CREATE TABLE IF NOT EXISTS file_changes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    commit_hash TEXT,
    file_path TEXT,
    insertions INTEGER,
    deletions INTEGER,
    FOREIGN KEY (commit_hash) REFERENCES commits(hash)
)
''')

cursor.execute('''
CREATE TABLE IF NOT EXISTS tutorials (
    id INTEGER PRIMARY KEY,
    name TEXT,
    description TEXT,
    created_date TEXT,
    last_modified_date TEXT
)
''')

# Parse git log
result = subprocess.run(['git', 'log', '--format=%H|%an|%ae|%ad|%s', '--date=iso', '--all'], 
                       capture_output=True, text=True, cwd='.')

commits_data = []
for line in result.stdout.strip().split('\n'):
    if '|' in line:
        parts = line.split('|', 4)
        if len(parts) >= 5:
            commits_data.append({
                'hash': parts[0],
                'author_name': parts[1],
                'author_email': parts[2],
                'date': parts[3],
                'message': parts[4]
            })

# Get stats for each commit
for commit in commits_data:
    result = subprocess.run(['git', 'show', '--numstat', '--format=%P', commit['hash']],
                           capture_output=True, text=True, cwd='.')
    lines = result.stdout.strip().split('\n')
    parent_hash = lines[0] if lines else ''
    
    total_insertions = 0
    total_deletions = 0
    file_count = 0
    
    for line in lines[1:]:
        if line and '\t' in line:
            parts = line.split('\t')
            if len(parts) >= 3:
                try:
                    insertions = int(parts[0]) if parts[0] != '-' else 0
                    deletions = int(parts[1]) if parts[1] != '-' else 0
                    file_path = parts[2]
                    total_insertions += insertions
                    total_deletions += deletions
                    file_count += 1
                    
                    cursor.execute('''
                        INSERT OR IGNORE INTO file_changes (commit_hash, file_path, insertions, deletions)
                        VALUES (?, ?, ?, ?)
                    ''', (commit['hash'], file_path, insertions, deletions))
                except ValueError:
                    pass
    
    cursor.execute('''
        INSERT OR REPLACE INTO commits (hash, author_name, author_email, date, message, parent_hash, 
                           files_changed, insertions, deletions)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', (commit['hash'], commit['author_name'], commit['author_email'], commit['date'],
          commit['message'], parent_hash, file_count, total_insertions, total_deletions))

conn.commit()
print(f"Inserted {len(commits_data)} commits")
conn.close()

