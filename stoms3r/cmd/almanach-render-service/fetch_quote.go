package main

import (
	"math/rand"
)

// quotePool is a local pool of quotes for offline operation.
// Can be replaced with an API (e.g., quotable.io) in the future.
var quotePool = []QuoteData{
	{Text: "The unexamined life is not worth living.", Author: "Socrates", Source: "Apology"},
	{Text: "In the middle of difficulty lies opportunity.", Author: "Albert Einstein"},
	{Text: "The only way to do great work is to love what you do.", Author: "Steve Jobs"},
	{Text: "Simplicity is the ultimate sophistication.", Author: "Leonardo da Vinci"},
	{Text: "What we know is a drop, what we don't know is an ocean.", Author: "Isaac Newton"},
	{Text: "The best time to plant a tree was 20 years ago. The second best time is now.", Author: "Chinese Proverb"},
	{Text: "Not all those who wander are lost.", Author: "J.R.R. Tolkien", Source: "The Fellowship of the Ring"},
	{Text: "Any sufficiently advanced technology is indistinguishable from magic.", Author: "Arthur C. Clarke"},
}

// fetchQuote returns a random quote from the local pool.
func fetchQuote() *QuoteData {
	if len(quotePool) == 0 {
		return nil
	}
	return &quotePool[rand.Intn(len(quotePool))]
}
