#include <Rcpp.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

int get_nterms (std::string fpath);
std::vector<std::string> split (std::string input, char delim); 

//' Rcpp Parse Topic Counts File --word-topic-counts
//'
//' Parses the file generated by Mallet with the --word-topic-counts flag.
//' The format is a whitespace delimited file, where each line has: termnumber term topic:count topic:count ...
//' Refer to the README for this package for more information about the word-topic-counts file.
//' Needs to know the number of topics. Returns the topic terms matrix.
//'
//' @param fpath A string specifying path to the word-topic-counts text file.
//' @param ntopics An integer representing the number of topics.
//' @return topicterms A matrix with topic terms, terms as colnames.
//' @export
// [[Rcpp::export]]
Rcpp::NumericMatrix rcpp_parse_word_topic_counts (std::string fpath, int ntopics)
{
    int nterms = get_nterms(fpath);
    int linecounter = 0;
    Rcpp::NumericMatrix results (ntopics, nterms);
    Rcpp::StringVector terms;


    std::ifstream infile(fpath);

    if (infile.is_open())
    {
	std::string line;
	while (getline(infile, line))
	{
	    if (linecounter % 100000 == 0 && linecounter != 0)
		Rcpp::Rcout << std::setprecision(7) << (double)linecounter / 1000000 << " million lines" << std::flush << std::endl; 

	    std::vector<std::string> elem = split(line, ' ');
	    std::string term = elem[1];
	    terms.push_back(term);
	    for (size_t i = 2; i < elem.size(); i++)
	    {
		std::vector<std::string> temp = split(elem[i], ':');
		int topic = std::stoi(temp[0]);
		int count = std::stoi(temp[1]);
		results(topic, linecounter) = count;
	    }// for each element > 1

	    linecounter++;
	}//while lines

	infile.close();
    }//if file is open
    Rcpp::colnames(results) = terms;
    return results;
} //rcpp_parse_topic_counts

int get_nterms (std::string fpath)
{
    int nterms = 0;
    std::ifstream infile(fpath);
    if (infile.is_open())
    {
	std::string line;
	while (getline(infile, line))
	{
	    nterms++;
	}//for each line
    }//if 
    return nterms;
}//get_nterms

std::vector<std::string> split (std::string input, char delim) 
{
    std::vector<std::string> result;
    std::stringstream ss;
    ss.str(input);
    std::string item;

    while (getline (ss, item, delim)) {
	result.push_back (item);
    }

    return result;
} // split
