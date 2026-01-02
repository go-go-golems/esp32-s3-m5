#!/usr/bin/env python3
"""
Analyze project history and generate insights.
"""
import sqlite3
from collections import defaultdict
from datetime import datetime

conn = sqlite3.connect('project_history.db')
cursor = conn.cursor()

print("=" * 80)
print("ESP32-S3-M5 PROJECT ANALYSIS")
print("=" * 80)

# 1. Timeline analysis
print("\n1. TIMELINE ANALYSIS")
print("-" * 80)
cursor.execute('SELECT MIN(date), MAX(date) FROM commits')
start_date, end_date = cursor.fetchone()
start_dt = datetime.fromisoformat(start_date.replace(' -0500', ''))
end_dt = datetime.fromisoformat(end_date.replace(' -0500', ''))
duration = (end_dt - start_dt).days
print(f"Project duration: {duration} days ({start_dt.date()} to {end_dt.date()})")

# Commits per day
cursor.execute('''
    SELECT DATE(date) as day, COUNT(*) as count
    FROM commits
    GROUP BY day
    ORDER BY day
''')
print("\nCommits per day:")
for row in cursor.fetchall():
    print(f"  {row[0]}: {row[1]} commits")

# 2. Tutorial progression
print("\n2. TUTORIAL PROGRESSION")
print("-" * 80)
cursor.execute('''
    SELECT id, name, created_date, last_modified_date
    FROM tutorials
    ORDER BY id
''')
tutorials = cursor.fetchall()
print(f"Total tutorials: {len(tutorials)}")
for tut in tutorials:
    print(f"  Tutorial {tut[0]:04d}: Created {tut[2]}, Last modified {tut[3]}")

# 3. Most active files
print("\n3. MOST ACTIVELY CHANGED FILES")
print("-" * 80)
cursor.execute('''
    SELECT file_path, SUM(insertions + deletions) as total_changes, COUNT(*) as commits
    FROM file_changes
    GROUP BY file_path
    ORDER BY total_changes DESC
    LIMIT 20
''')
print("Top 20 files by changes:")
for row in cursor.fetchall():
    print(f"  {row[0]}: {row[1]} changes across {row[2]} commits")

# 4. File type analysis
print("\n4. FILE TYPE ANALYSIS")
print("-" * 80)
cursor.execute('''
    SELECT 
        CASE 
            WHEN file_path LIKE '%.c' THEN 'C'
            WHEN file_path LIKE '%.cpp' THEN 'C++'
            WHEN file_path LIKE '%.h' THEN 'Header'
            WHEN file_path LIKE '%.md' THEN 'Markdown'
            WHEN file_path LIKE '%.py' THEN 'Python'
            WHEN file_path LIKE '%.sh' THEN 'Shell'
            WHEN file_path LIKE '%.yaml' OR file_path LIKE '%.yml' THEN 'YAML'
            WHEN file_path LIKE '%.txt' THEN 'Text'
            WHEN file_path LIKE '%.json' THEN 'JSON'
            WHEN file_path LIKE '%.csv' THEN 'CSV'
            WHEN file_path LIKE 'CMakeLists.txt' THEN 'CMake'
            WHEN file_path LIKE 'sdkconfig%' THEN 'Config'
            ELSE 'Other'
        END as file_type,
        COUNT(*) as file_count,
        SUM(insertions + deletions) as total_changes
    FROM file_changes
    GROUP BY file_type
    ORDER BY total_changes DESC
''')
print("Changes by file type:")
for row in cursor.fetchall():
    print(f"  {row[0]}: {row[1]} files, {row[2]} total changes")

# 5. Commit message patterns
print("\n5. COMMIT MESSAGE PATTERNS")
print("-" * 80)
cursor.execute('SELECT message FROM commits')
messages = [row[0] for row in cursor.fetchall()]

patterns = defaultdict(int)
for msg in messages:
    if msg.startswith('Tutorial'):
        patterns['Tutorial'] += 1
    elif '001' in msg:
        patterns['Ticket/Tutorial Reference'] += 1
    elif 'Diary' in msg:
        patterns['Diary'] += 1
    elif 'Docs' in msg:
        patterns['Documentation'] += 1
    elif 'Fix' in msg or 'fix' in msg:
        patterns['Bug Fix'] += 1
    elif 'Add' in msg or 'add' in msg:
        patterns['Feature Addition'] += 1
    elif 'chore' in msg.lower():
        patterns['Chore'] += 1

print("Commit types:")
for pattern, count in sorted(patterns.items(), key=lambda x: -x[1]):
    print(f"  {pattern}: {count}")

# 6. Directory structure analysis
print("\n6. DIRECTORY STRUCTURE ANALYSIS")
print("-" * 80)
cursor.execute('''
    SELECT 
        CASE 
            WHEN file_path LIKE 'ttmp/%' THEN 'Documentation (ttmp)'
            WHEN file_path LIKE '000%' THEN 'Tutorial Projects'
            WHEN file_path LIKE 'components/%' THEN 'Components'
            WHEN file_path LIKE 'imports/%' THEN 'Imports'
            WHEN file_path LIKE 'main/%' THEN 'Main Source'
            ELSE 'Other'
        END as directory,
        COUNT(DISTINCT file_path) as file_count,
        SUM(insertions + deletions) as total_changes
    FROM file_changes
    GROUP BY directory
    ORDER BY total_changes DESC
''')
print("Changes by directory:")
for row in cursor.fetchall():
    print(f"  {row[0]}: {row[1]} files, {row[2]} total changes")

conn.close()

