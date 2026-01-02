#!/bin/bash
# Live reloading dashboard for ESP32-S3-M5 project history analysis

DB_PATH="project_history.db"
REFRESH_INTERVAL=2

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to get database stats
get_stats() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers on
SELECT 
    COUNT(*) as "Total Commits",
    SUM(insertions) as "Total Insertions",
    SUM(deletions) as "Total Deletions",
    (SELECT COUNT(DISTINCT file_path) FROM file_changes) as "Unique Files"
FROM commits;
EOF
}

# Function to get tutorial stats
get_tutorial_stats() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers on
SELECT 
    id as "ID",
    name as "Name",
    DATE(created_date) as "Created",
    DATE(last_modified_date) as "Last Modified"
FROM tutorials
ORDER BY id;
EOF
}

# Function to get recent commits
get_recent_commits() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers off
SELECT 
    substr(date, 1, 19) as date,
    substr(message, 1, 60) as message,
    insertions || '/' || deletions as changes
FROM commits
ORDER BY date DESC
LIMIT 10;
EOF
}

# Function to get commit activity by day
get_daily_activity() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers on
SELECT 
    substr(date, 1, 10) as "Date",
    COUNT(*) as "Commits",
    SUM(insertions) as "Insertions",
    SUM(deletions) as "Deletions"
FROM commits
GROUP BY substr(date, 1, 10)
ORDER BY substr(date, 1, 10) DESC
LIMIT 10;
EOF
}

# Function to get file type breakdown
get_file_types() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers on
SELECT 
    CASE 
        WHEN fc.file_path LIKE '%.c' THEN 'C'
        WHEN fc.file_path LIKE '%.cpp' THEN 'C++'
        WHEN fc.file_path LIKE '%.h' THEN 'Header'
        WHEN fc.file_path LIKE '%.md' THEN 'Markdown'
        WHEN fc.file_path LIKE '%.py' THEN 'Python'
        WHEN fc.file_path LIKE '%.sh' THEN 'Shell'
        WHEN fc.file_path LIKE '%.yaml' OR fc.file_path LIKE '%.yml' THEN 'YAML'
        WHEN fc.file_path LIKE '%.json' THEN 'JSON'
        WHEN fc.file_path LIKE 'CMakeLists.txt' THEN 'CMake'
        WHEN fc.file_path LIKE 'sdkconfig%' THEN 'Config'
        ELSE 'Other'
    END as "Type",
    COUNT(DISTINCT fc.file_path) as "Files",
    SUM(fc.insertions + fc.deletions) as "Total Changes"
FROM file_changes fc
GROUP BY Type
ORDER BY "Total Changes" DESC
LIMIT 10;
EOF
}

# Function to get most active files
get_active_files() {
    sqlite3 "$DB_PATH" <<EOF
.mode column
.headers on
SELECT 
    substr(fc.file_path, 1, 50) as "File",
    SUM(fc.insertions + fc.deletions) as "Changes",
    COUNT(*) as "Commits"
FROM file_changes fc
GROUP BY fc.file_path
ORDER BY Changes DESC
LIMIT 10;
EOF
}

# Function to display dashboard
show_dashboard() {
    clear
    echo -e "${CYAN}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC} ${GREEN}ESP32-S3-M5 Project History Dashboard${NC}${CYAN}                                                      ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    echo -e "${YELLOW}📊 PROJECT STATISTICS${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_stats
    echo ""
    
    echo -e "${YELLOW}📅 DAILY ACTIVITY (Last 10 Days)${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_daily_activity
    echo ""
    
    echo -e "${YELLOW}📚 TUTORIALS PROGRESS${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_tutorial_stats
    echo ""
    
    echo -e "${YELLOW}📁 FILE TYPE BREAKDOWN${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_file_types
    echo ""
    
    echo -e "${YELLOW}🔥 MOST ACTIVE FILES${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_active_files
    echo ""
    
    echo -e "${YELLOW}📝 RECENT COMMITS (Last 10)${NC}"
    echo -e "${BLUE}──────────────────────────────────────────────────────────────────────────────${NC}"
    get_recent_commits
    echo ""
    
    echo -e "${MAGENTA}Last updated: $(date '+%Y-%m-%d %H:%M:%S') | Refresh: ${REFRESH_INTERVAL}s | Press Ctrl+C to exit${NC}"
}

# Check if database exists
if [ ! -f "$DB_PATH" ]; then
    echo -e "${RED}Error: Database $DB_PATH not found!${NC}"
    echo "Run: python3 debug-history-scripts/01-populate-database.py"
    exit 1
fi

# Main loop
trap 'echo -e "\n${GREEN}Dashboard stopped.${NC}"; exit 0' INT TERM

while true; do
    show_dashboard
    sleep "$REFRESH_INTERVAL"
done

