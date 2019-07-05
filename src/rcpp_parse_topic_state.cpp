#include <Rcpp.h>
#include <iostream>
#include <cstring> // for gzstream
#include <iomanip>
#include "gzstream.h" // add -lz to compile

struct Info {
    int ntopics = 0;
    int nterms = 0;
    int ndocs = 0;
};

void get_info (std::string fpath, Info *info);


//' Rcpp Parse Topic State File --output-state
//'
//' Parses the file generated by Mallet with the --output-state flag.
//' The file is generally millions of lines long and is gzipped. 
//' The first three lines are meta information about the model.
//' Refer to the package README to see more about the file. 
//' This function is set up so that you can specify which information you want to extract from the state file.
//' If you have modeled a corpus with many millions of files, you will not be able to store the document topics matrix into memory.
//' In that case, set the dtflag to 0. 
//' To still extract the document topic matrix, set the dtfile parameter to the file you want to save the gzipped document topics matrix.
//' Writing it to file is done document by document and will not have any memory issues regardless of size, though may take up a lot of space on the disk.
//' This file contains all the information needed to run ldavis, look at the vignette for creating the json for ldavis with the information form the ifle.
//'
//' @param fpath A string specifying path to the output-state file.
//' @param termflag An integer (default = 1) set to 1 if you want to extract the terms and their frequencies.
//' @param docflag An integer (default = 1) set to 1 if you want to extract the document names and their token counts.
//' @param ttflag An integer (default = 1) set to 1 if you want to extract the topic terms matrix.
//' @param dtflag An integer (default = 1) set to 1 if you want to extract the document topic matrix. 
//' @param dtfile A string (default = "") specifying path to the output file you want to write the document topics to, leave blank if you don't want to write the doc topics to a file.
//' @return A list with these components:
//' \describe{
//'   \item{terms}{Character vector of the terms}
//'   \item{term_freqs}{Numeric vector of occurances for each term}
//'   \item{docs}{Character vector of the document names}
//'   \item{doc_lens}{Numeric vector of the number of tokens for each document}
//'   \item{topic_terms}{A matrix of topic terms (KxV)}
//'   \item{doc_topics}{A matrix of document topics (DxK)}
//'   \item{doc_topics_fpath}{A string with the path to the doc topics gzipped file}
//' }
//' @export
// [[Rcpp::export]]
Rcpp::List rcpp_parse_topic_state (std::string fpath, int termflag=1, int docflag=1, int ttflag=1, int dtflag=1, std::string dtfile="")
{
    struct Info info;
    get_info(fpath.c_str(), &info);

    //init data structures
    Rcpp::StringVector terms;
    Rcpp::NumericVector wf;
    Rcpp::StringVector docs;
    Rcpp::NumericVector dc;

    if (termflag == 1)
    {
	for (int i = 0; i < info.nterms; i++)
	{
	    wf.push_back(0);
	    terms.push_back("");
	}
    }

    if (docflag == 1) 
    {
	for (int i = 0; i < info.ndocs; i++)
	{
	    dc.push_back(0);
	    docs.push_back("");
	}
    }

    int tdim1 = 0;
    int tdim2 = 0;
    int ddim1 = 0;
    int ddim2 = 0;

    if (ttflag == 1)
    {
	tdim1 = info.ntopics;
	tdim2 = info.nterms;
    }

    if (dtflag == 1)
    {
	ddim1 = info.ndocs;
	ddim2 = info.ntopics;
    }


    Rcpp::NumericMatrix topicterms(tdim1,tdim2);
    Rcpp::NumericMatrix doctopics(ddim1,ddim2);

    igzstream infile(fpath.c_str());
    ogzstream ofile;

    if (dtfile != "")
	ofile.open(dtfile.c_str());

    //-------     MAIN ---------------------------------------------------------------
    // use getline so that we can skip over the first three lines of the file
    std::string line;
    int linecount = 0;
    std::vector <int> doctopic (info.ndocs, 0);
    int past = -1;
    while (std::getline(infile, line))
    {
	if (linecount > 2)
	{

	    if (linecount % 100000 == 0 && linecount != 0)
		    Rcpp::Rcout << std::setprecision(7) << (double)linecount / 1000000 << " million lines" << std::flush << std::endl;

	    int docindex, pos, typeindex, topic;
	    std::string source, type;
	    std::istringstream iss(line);
	    iss >> docindex >> source >> pos >> typeindex >> type >> topic;

	    if (past == -1)
		past = docindex;

	    // Word Frequencies
	    if (termflag == 1)
	    {
		terms[typeindex] = type;
		wf[typeindex]++;
	    }

	    // Doc Token Counts
	    if (docflag == 1)
	    {
		docs[docindex] = source;
		dc[docindex]++;
	    }

	    // Topic Terms
	    if (ttflag == 1)
		topicterms(topic, typeindex)++;

	    // Doc Topics
	    if (dtflag == 1)
		doctopics(docindex, topic)++;

	    if (dtfile != "")
	    {
		if (docindex != past)
		{
		    ofile << doctopic[0]; //so no trailing tab
		    for (int i = 1; i < info.ntopics; i++)
			ofile << " " << doctopic[i];
		    ofile << "\n";

		    // clear doctopic
		    for (int i = 0; i < info.ntopics; i++)
			doctopic[i] = 0;
		}// if topic != past

		past = docindex;
		doctopic[topic]++;
	    }
	}// if past the first three lines (comments)
	linecount++;
    }

    if (dtfile != "")
    {
	ofile << doctopic[0];
	for (int i = 1; i < info.ntopics; i++)
	    ofile << " " << doctopic[i];
	ofile << "\n";
    }

    if (termflag == 1 && ttflag == 1)
	Rcpp::colnames(topicterms) = terms;
    if (docflag == 1 && dtflag == 1)
	Rcpp::rownames(doctopics) = docs;

    return Rcpp::List::create( 
	    Rcpp::Named("terms")  = terms,
	    Rcpp::Named("term_freqs")  = wf,
	    Rcpp::Named("docs") = docs,
	    Rcpp::Named("doc_lens") = dc,
	    Rcpp::Named("topic_terms") = topicterms,
	    Rcpp::Named("doc_topics") = doctopics,
	    Rcpp::Named("doc_topics_fpath") = dtfile
	    ) ;
}


void get_info (std::string fpath, Info *info)
{
    igzstream infile(fpath.c_str());

    // use getline so that we can skip over the first three lines of the file
    std::string line;
    int linecount = 0;
    while (std::getline(infile, line))
    {
	if (linecount > 2)
	{
	    if (linecount % 100000 == 0 && linecount != 0)
		    Rcpp::Rcout << std::setprecision(7) << (double)linecount / 1000000 << " million lines" << std::flush << std::endl;
	    int docindex, pos, typeindex, topic;
	    std::string source, type;
	    std::istringstream iss(line);
	    iss >> docindex >> source >> pos >> typeindex >> type >> topic;
	    if (topic > info->ntopics)
		info->ntopics = topic;
	    if (docindex > info->ndocs)
		info->ndocs = docindex;
	    if (typeindex > info->nterms)
		info->nterms = typeindex;
	}// if past the first three lines (comments)

	linecount++;
    } // while each line
    info->nterms++;
    info->ntopics++;
    info->ndocs++;

    return; 
}// get_info
