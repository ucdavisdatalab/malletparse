# Mallet Parse DSI #

This package is meant as a conveniant and fast way of parsing the output files from a MALLET topic model. 
Notably it parses the text file outputs and does not use [rJava](https://cran.r-project.org/web/packages/rjava/index.html) to parse the java binary objects. 
Written in Rcpp and designed to work even for large files.
Also contains custom functions for creating the json part of [LDAvis](https://github.com/cspievert/LDAvis) on large doc topic matrices.

## Installation Instructions ##

Clone the bitbucket repository. Through the command line run R CMD build and R CMD INSTALL. When installed refer to the **vignette** to see how to use it.

Alternatively
```
devtools::install_bitbucket("digitalscholarship/malletparsedsi")
```

## Mallet Output Files ##

When running a topic model with Mallet there are several different outputs available.  
- **model binary file**, --output-model <filename>
- **topic keys file**, --output-topic-keys <filename>
- **state file**, --output-state <filename.gz>
- **doc topics file**, --output-doc-topics <filename>
- **word topic counts**, --word-topic-counts <filename>
- **topic word weights**, --topic-word-weights <filename>  

For example, run this command to generate all the outputs listed above:

```
mallet train-topics --input reviews.mallet --output-model model.bin --output-topic-keys topic-keys.dat --output-doc-topics doc-topics.dat --output-state state.gz --word-topic-counts-file word-topics.dat --topic-word-weights word-weights.dat --num-iterations 1000
```

These files can be parsed with their corresponding functions from this package:  
- **rcpp_parse_topic_state**
- **rcpp_parse_doc_topics**
- **rcpp_parse_word_topic_counts**
- **rcpp_parse_topic_word_weights**  

## LDAvis for large models ##

Sometimes the model you have trained is too big for the **LDAvis** package to handle. 
This will occur if the number of documents you have is very large (>10 million), as LDAvis expects the doc topics in the form of a numeric matrix and performs a method (for each column in dt, for each value multiply value by number of tokens in that document, add new value to sum) then divide each sum by the sum of all the sums to estimate the topic proportions in the corpus.
Additionally, if the number of terms you have is very large (>1 million) you can have problems.
Generally, however, its simple to reduce the number of terms before running the model, or by removing columns from the topic terms matrix before passing the data to LDAvis.  

To handle large doc topics it is simpler to calculate the topic frequency on your own and pass that into a modified LDAvis function. 
To calculate topic frequency you can sample rows from dt, or you can use the appropriate part from the **rcpp_parse_doc_topics** function from this package.  

For example a workflow for generating the LDAvis visualization could look like:  

1. Get Words, Word Frequencies, Doc Lengths, Topic Terms, and Doc Topics from the state file using **rcpp_parse_topic_state**
2. Compute the topic frequencies using **rcpp_parse_doc_topics** on the Doc Topics File (can be the mallet output doc topics or the gzipped result of 1)  
3. call the modified createJSON function from this package: **no_doc_topics_createJSON.R**
4. visualize with the LDAvis **serVis** function  

For code look at the vignette for this package.

##### Model File #####

Saved from MALLET train topics with the **--output-model** flag. This is a binary file and not parsed with this package.  

##### Topic Keys File #####

Saved from MALLET train topics with the **--output-topic-keys** flag. This contains the top words per topic. The number of words to output defaults to 20 but can be configured, look at train-topics --help for more information.
Each line has topicid, alpha, top 20 words.

The first few lines:
```
0	0.5	the and are that his from film with not who more when world one there was has for which dark 
1	0.5	the and his that with him for their who about life has movie its will into when out man they 
2	0.5	the and movie you with it's out that bad like get but there's for all this off even one good 
3	0.5	the her and she with for from but love that which when film his one who story while woman music 
4	0.5	the and his with for action that has but from are who family than him one old have films some 
```
Not much to parse here, but nice to have to check your work or to quickly get a sense of a topic.

##### Topic State File #####

Saved from MALLET train topics with the **--output-state** flag. This contains all the information you need for LDAvis. Usually very large and is gzipped by MALLET.
Contains the words of the corpus and their topic assignments. Also contains the document that the word appeared in.  

The first few lines look like:  

```
#doc source pos typeindex type topic
#alpha : 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 
#beta : 0.01
0 ./reviews/1150.txt 0 0 when 6
0 ./reviews/1150.txt 1 1 originally 5
0 ./reviews/1150.txt 2 2 saw 8
0 ./reviews/1150.txt 3 3 the 3
0 ./reviews/1150.txt 4 4 trailer 6
0 ./reviews/1150.txt 5 5 for 8
0 ./reviews/1150.txt 6 6 analyze 3
```

From this file we are able to extract the **terms**, **term frequencies**, **documents**, **document token counts** (number of words per document), **topic terms matrix** and **doc topics matrix**.  

The document topics matrix can be arbitrarily large as its dimensions is dependent on the number of documents used to train the model which can lead to problems of memory usage.
For example, a topic model on a corpus of tweets may contain 100,000,000 unique documents and 50 topics. This results in a matrix with 5 billion elements, which is usually too large to even fit in a standard R vector. Additionally, the RAM usage will be hard to deal with. 
When parsing this file be aware of how many documents you have in the model, if it is a large number, consider setting the doc topics flag to 0 and instead writing the doc topics to file, or using the Mallet output doc topics flag.


##### Doc Topics File #####

Saved from MALLET train topic with the **--output-doc-topics** flag. 
See parameters for this file with train-topics --help. 
Each line has the document id (starting at 0, then the document name, then the score for each topic.  

The first 5 lines look like (Note that I shortened this output manually to only display the first two topics and then an elipses, number of columns depends on number of topics.):

```
0	file:/home/avkoehl/programs/mallet-tests/./reviews/1150.txt	0.013853904282115869	0.008816120906801008 ...	
1	file:/home/avkoehl/programs/mallet-tests/./reviews/871.txt	0.035507246376811595	0.02681159420289855  ...
2	file:/home/avkoehl/programs/mallet-tests/./reviews/491.txt	0.001607717041800643	0.04126473740621651  ...
3	file:/home/avkoehl/programs/mallet-tests/./reviews/1237.txt	0.0586283185840708	0.02101769911504425  ...
4	file:/home/avkoehl/programs/mallet-tests/./reviews/1212.txt	0.00125	0.03625	...
```

From this file we are able to extract **top topics per document**, and **top documents per topic**.
And, given the document token lengths the topic frequencies can be calculated.

##### Word Topic Counts #####

Saved from MALLET train topics with the **--word-topic-counts** flag.
Each line has the termid, term, and then a series of topic:count elements for that word.  

The first five lines:  
```
0 when 6:794 5:674 7:487 4:404 9:298 1:208 0:194 8:162 2:33
1 originally 6:26 5:26 4:18 0:1
2 saw 8:174 5:78
3 the 8:17048 9:9126 0:8765 1:8735 5:7261 3:7066 2:5567 6:5135 4:4666 7:2958
4 trailer 6:75 8:51
```

From this file we can get the **terms**,  **term frequencies**, and **topic terms matrix**.
To normalize the topic term matrix just divide the count for each term by the sum of counts for that topic.

##### Topic Word Weights #####

Saved from MALLET train topics with the **--topic-word-weights** flag.
Each line has the topicid, term, and then the weight for that term in that topic. 
The weight seems to be the count + the beta hyperparameter for that word.  

The first five lines:  

```
0	when	194.01
0	originally	1.01
0	saw	0.01
0	the	8765.01
0	trailer	0.01
```

From this file we can get the **terms**,  **term frequencies**, and **topic terms matrix**.
To normalize the topic term matrix just divide the count for each term by the sum of counts for that topic.

## Contact ##
 
To contact the maintainer email Arthur Koehl at avkoehl at ucdavis.edu
