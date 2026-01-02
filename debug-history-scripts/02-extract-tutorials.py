#!/usr/bin/env python3
"""
Extract tutorial information from commit messages and populate tutorials table.
"""
import sqlite3
import re

conn = sqlite3.connect('project_history.db')
cursor = conn.cursor()

# Extract tutorial numbers from commit messages
cursor.execute('''
    SELECT hash, message FROM commits
    WHERE message LIKE '%001%' OR message LIKE '%Tutorial%'
    ORDER BY date
''')

tutorials = {}
for row in cursor.fetchall():
    hash_val, message = row
    # Extract tutorial numbers
    matches = re.findall(r'00\d{2}', message)
    for match in matches:
        if match not in tutorials:
            tutorials[match] = {'first_seen': hash_val, 'commits': []}
        tutorials[match]['commits'].append(hash_val)

# Get tutorial info
for tutorial_num in sorted(tutorials.keys()):
    cursor.execute('''
        SELECT date FROM commits WHERE hash = ?
    ''', (tutorials[tutorial_num]['first_seen'],))
    result = cursor.fetchone()
    if result:
        tutorials[tutorial_num]['created_date'] = result[0]
    
    if tutorials[tutorial_num]['commits']:
        placeholders = ','.join(['?' for _ in tutorials[tutorial_num]['commits']])
        cursor.execute(f'''
            SELECT date FROM commits 
            WHERE hash IN ({placeholders})
            ORDER BY date DESC LIMIT 1
        ''', tutorials[tutorial_num]['commits'])
        result = cursor.fetchone()
        if result:
            tutorials[tutorial_num]['last_modified_date'] = result[0]

# Insert tutorials
for tutorial_num, info in tutorials.items():
    cursor.execute('''
        INSERT OR REPLACE INTO tutorials (id, name, created_date, last_modified_date)
        VALUES (?, ?, ?, ?)
    ''', (int(tutorial_num), f'Tutorial {tutorial_num}', 
          info.get('created_date', ''), info.get('last_modified_date', '')))

conn.commit()
print(f"Inserted {len(tutorials)} tutorials")
conn.close()

