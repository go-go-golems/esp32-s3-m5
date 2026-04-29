package main

import (
	"math/rand"
)

var wordPool = []WordData{
	{Word: "serendipity", Definition: "The occurrence of events by chance in a happy or beneficial way.", PartOfSpeech: "noun", Example: "A fortunate stroke of serendipity."},
	{Word: "ephemeral", Definition: "Lasting for a very short time.", PartOfSpeech: "adjective", Example: "Fashions are ephemeral: new ones regularly emerge."},
	{Word: "petrichor", Definition: "A pleasant smell that frequently accompanies the first rain after a long period of warm, dry weather.", PartOfSpeech: "noun"},
	{Word: "mellifluous", Definition: "A sound that is pleasingly smooth and musical to hear.", PartOfSpeech: "adjective"},
	{Word: "lacuna", Definition: "An unfilled space or gap; a missing part.", PartOfSpeech: "noun", Example: "There are lacunae in our knowledge of the period."},
	{Word: "syzygy", Definition: "A roughly straight-line configuration of three or more celestial bodies in a gravitational system.", PartOfSpeech: "noun"},
	{Word: "apricity", Definition: "The warmth of the sun in winter.", PartOfSpeech: "noun"},
}

// fetchWord returns a random word from the local pool.
func fetchWord() *WordData {
	if len(wordPool) == 0 {
		return nil
	}
	return &wordPool[rand.Intn(len(wordPool))]
}
