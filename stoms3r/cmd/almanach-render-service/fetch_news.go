package main

// newsFeed is a hardcoded fallback for development.
// Phase 4d will replace this with RSS parsing.

var fallbackHeadlines = []NewsItem{
	{Headline: "Placeholder headline 1", Source: "BBC", Summary: "Replace with real RSS feed data in Phase 4d."},
	{Headline: "Placeholder headline 2", Source: "Reuters", Summary: "Wire fetchers/news.go to parse RSS feeds."},
}

// fetchNews returns placeholder news items.
// TODO: Implement RSS parsing for BBC, Reuters, etc.
func fetchNews() *NewsData {
	return &NewsData{Items: fallbackHeadlines}
}
