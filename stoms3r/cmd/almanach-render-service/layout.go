package main

import (
	"encoding/json"
	"fmt"
	"time"
)

// Layout represents the full almanac page layout sent to the SPA.
type Layout struct {
	Version   int      `json:"almanach_studio_version"`
	ExportedAt string   `json:"exported_at"`
	Theme     string   `json:"theme"`
	PaperWidth int     `json:"paperWidth"`
	BodyScale  float64 `json:"bodyScale"`
	FeedLines  int     `json:"feedLines"`
	Blocks    []Block  `json:"blocks"`
}

// Block represents a single almanac block (title, weather, news, etc.).
type Block struct {
	ID   string          `json:"id"`
	Type string          `json:"type"`
	Data json.RawMessage `json:"data"`
}

// Data types for each block type.

type TitleData struct {
	Title    string `json:"title"`
	Subtitle string `json:"subtitle"`
}

type DateData struct {
	Date string `json:"date"`
	Day  string `json:"day"`
}

type WeatherData struct {
	Temp      string `json:"temp"`
	High      string `json:"high"`
	Low       string `json:"low"`
	Condition string `json:"condition"`
	Humidity  string `json:"humidity"`
	Wind      string `json:"wind"`
}

type NewsData struct {
	Items []NewsItem `json:"items"`
}

type NewsItem struct {
	Headline string `json:"headline"`
	Source   string `json:"source"`
	Summary  string `json:"summary"`
}

type PlanData struct {
	Label string     `json:"label"`
	Items []PlanItem `json:"items"`
}

type PlanItem struct {
	Time string `json:"time"`
	Text string `json:"text"`
	Done bool   `json:"done"`
}

type QuoteData struct {
	Text   string `json:"text"`
	Author string `json:"author"`
	Source string `json:"source,omitempty"`
}

type WordData struct {
	Word          string `json:"word"`
	Definition    string `json:"definition"`
	PartOfSpeech  string `json:"partOfSpeech"`
	Example       string `json:"example,omitempty"`
}

type HistoryData struct {
	Year  string `json:"year"`
	Event string `json:"event"`
}

type HabitsData struct {
	Items []HabitItem `json:"items"`
}

type HabitItem struct {
	Name string `json:"name"`
	Done bool   `json:"done"`
}

type DidYouKnowData struct {
	Text string `json:"text"`
}

type NoteData struct {
	Title   string `json:"title"`
	Content string `json:"content"`
}

type MoodData struct {
	Mood    int    `json:"mood"`
	Energy  int    `json:"energy"`
	Note    string `json:"note,omitempty"`
}

type ReadingData struct {
	Title   string      `json:"title"`
	Author  string      `json:"author"`
	Pages   string      `json:"pages"`
	Current ReadingProgress `json:"current"`
}

type ReadingProgress struct {
	Page     int `json:"page"`
	Total    int `json:"total"`
	Progress int `json:"progress"`
}

type ReflectionData struct {
	Prompt   string `json:"prompt"`
	Response string `json:"response"`
}

// blockID counter for generating unique block IDs.
var blockCounter int

func nextBlockID() string {
	blockCounter++
	return fmt.Sprintf("b%d", blockCounter)
}

// newBlock creates a Block with a generated ID and JSON-encoded data.
func newBlock(typ string, data any) Block {
	raw, _ := json.Marshal(data)
	return Block{ID: nextBlockID(), Type: typ, Data: raw}
}

// dividerBlock creates a divider block (no data).
func dividerBlock() Block {
	return Block{ID: nextBlockID(), Type: "divider", Data: json.RawMessage("{}")}
}

// buildDefaultLayout constructs a layout using live data from fetchers.
func buildDefaultLayout(cfg Config) (*Layout, error) {
	now := time.Now()

	// Fetch data concurrently (TODO: Phase 4 — wire real fetchers)
	dateData := fetchDate(now)
	weatherData := fetchWeather(cfg)
	newsData := fetchNews()
	quoteData := fetchQuote()
	wordData := fetchWord()
	historyData := fetchHistory(now)

	var blocks []Block
	blocks = append(blocks,
		newBlock("title", TitleData{
			Title:    "Daily Almanac",
			Subtitle: formatDate(now),
		}),
		newBlock("date", dateData),
		dividerBlock(),
	)

	if weatherData != nil {
		blocks = append(blocks, newBlock("weather", weatherData))
	}

	if newsData != nil && len(newsData.Items) > 0 {
		blocks = append(blocks, newBlock("news", newsData))
	}

	if quoteData != nil {
		blocks = append(blocks, newBlock("quote", quoteData))
	}

	if wordData != nil {
		blocks = append(blocks, newBlock("word", wordData))
	}

	if historyData != nil {
		blocks = append(blocks, newBlock("history", historyData))
	}

	blocks = append(blocks,
		newBlock("did_you_know", DidYouKnowData{
			Text: "Honey never spoils. Archaeologists have found 3000-year-old honey in Egyptian tombs that was still edible.",
		}),
	)

	return &Layout{
		Version:    1,
		ExportedAt: now.UTC().Format(time.RFC3339),
		Theme:      cfg.DefaultTheme,
		PaperWidth: cfg.PaperWidth,
		BodyScale:  cfg.BodyScale,
		FeedLines:  cfg.FeedLines,
		Blocks:     blocks,
	}, nil
}

func formatDate(t time.Time) string {
	return t.Format("January 2, 2006")
}
