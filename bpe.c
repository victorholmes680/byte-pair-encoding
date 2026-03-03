#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "bpe.h"


typedef struct {
    Pair key;
    size_t value;
} Freq;

int compare_freq(const void *a, const void *b)
{
    const Freq *af = a;
    const Freq *bf = b;
    return (int)bf->value - (int)af->value;
}

typedef struct {
    Freq *items;
    size_t count;
    size_t capacity;
} Freqs;

typedef struct {
    uint32_t *items;
    size_t count;
    size_t capacity;
} Tokens;

void render_tokens(Pairs pairs, Tokens tokens)
{
    for(size_t i = 0; i < tokens.count; ++i)
    {
	uint32_t token = tokens.items[i];
	assert(token < pairs.count);
	if(pairs.items[token].l == token)
	{
	    // original character
	    printf("%c", token);
	} else {
	    printf("[%u]", token);
	}
    }
    printf("\n");
}

#define swap(Type, x, y)	\
do {				\
    Type t = (x);		\
    (x) = (y);			\
    (y) = t;			\
} while(0)			\

bool dump_pairs(const char *file_path, Pairs pairs)
{
    return write_entire_file(file_path, pairs.items, pairs.count*sizeof(*pairs.items));
}

int main()
{
    const char *text = "The quick brown fox jumps over the lazy dog";

    int text_size = strlen(text);

    Freq *freq = NULL;
    Pairs pairs = {0};
    Tokens tokens_in = {0};
    Tokens tokens_out = {0};

    // The following is Pair structure and the index means token
    // index	=> Pair
    // 0	=> { .l = 0, .r = ??? }
    // 1	=> { .l = 1, .r = ??? }
    // 2	=> { .l = 2, .r = ??? }
    // ...
    // 69	=> { .l = 69, .r = ??? }   
    // ...
    // 255	=> { .l = 255, .r = ??? }

    // use dynamic array pairs to accept the above info
    for(uint32_t i = 0; i < 256; ++i)
    {
	da_append(&pairs, ((Pair) {.l = i}));
    }

    // use dynamic array tokens_in to accept the input text
    for(int i = 0; i < text_size; ++i)
    {
	da_append(&tokens_in, text[i]);
    }

    for(;;)
    {
	// this operation is the bottomneck of the whole progress
	hmfree(freq);


	// region: construct the freq table
	for(size_t i = 0; i < tokens_in.count - 1; ++i)
	{
	    Pair pair = {
		.l = tokens_in.items[i];
		.r = tokens_in.items[i+1];
	    };

	    ptrdiff_t i = hmgeti(freq, pair);

	    // if not find the pair in the pairs table, make its default value be 1
	    if(i < 0) hmput(freq, pair, 1);
	    else freq[i].value += 1;
	}
	// endregion


	// region: select the pair whose freq is max
	ptrdiff_t max_index = 0;
	for(ptrdiff_t i = 1; i < hmlen(freq); ++i)
	{
	    if(freq[i].value > freq[max_index].value)
	    {
		max_index = i;
	    }
	}
	// endregion

	// when all of the pair just occur once, the progress is done
	if(freq[max_index].value <= 1) break;

	// append pairs with 
	da_append(&pairs, freq[max_index].key);

	for(size_t i = 0; i < tokens_in.count; )
	{
	    // current token is the last character of the tokens_in(input text)
	    if(i >= tokens_in.count-1)
	    {
		da_append(&tokens_out, tokens_in.items[i]);
		i += 1;
	    }
	    else
	    {
		Pair pair = {.l = tokens_in.items[i], .r = tokens_in.items[i+1]};
		if(memcmp(&pair, &freq[max_index].key, sizeof(pair)) == 0)
		{
		    da_append(&tokens_out, pairs.count-1);
		    i += 2;
		}
		else
		{
		    da_append(&tokens_out, tokens_in.items[i]);
		    i += 1;
		}
	    }
	}

	swap(Tokens, tokens_in, tokens_out);
    }

    if(!dump_pairs("pairs.bin", pairs)) return 1;
    return 0;
}

