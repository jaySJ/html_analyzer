#include <GetUrlContent.hpp>
#include <HtmlParser.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <omp.h>
#include <tuple>

namespace bfs = boost::filesystem;

#define DEBUG_PARALLEL_SECTIONS
#undef DEBUG_PARALLEL_SECTIONS

enum StatusCode {
    SUCCESS,
    FATAL_ERROR
};

int main(int argc, char * argv[])
{
    std::vector<std::string> urls;

    if (argc < 3) {
        std::cerr << "Expecting 2 arguments <path_to_text_file_with_urls> <num_threads> " << std::endl;
        return -1;
    }
    
    if (!bfs::exists(argv[1]))
       std::cerr <<"Could not find input file"<< argv[1] 
       << ". Please provide a text file with Urls" << std::endl;
    
    unsigned short numThreadsRequested = 1;
    int maxThreadsOnSystem = omp_get_num_procs();
    try {
        numThreadsRequested = boost::lexical_cast<unsigned short>(argv[2]);
        if (numThreadsRequested > maxThreadsOnSystem) {
            std::cout << "Cannot process with more threads than those available on this system (" 
                    << maxThreadsOnSystem << "). Limiting to " 
                    << maxThreadsOnSystem << " threads."
                    << std::endl;
            numThreadsRequested = maxThreadsOnSystem;
        }
    }
    catch (const boost::bad_lexical_cast &e)
    {
        std::cerr << "Could not understand the input for number of threads." << e.what() << '\n';
    }
    
    std::ifstream infile(argv[1]);    
    std::map<int, std::string> urlMap;
    try {
        if (infile.is_open())
        {
            int i = 0;
            std::string line;
            while (infile >> line)
            {
                urls.emplace_back(line);
                urlMap.insert({++i, line});
            }
        }
        infile.close();
    } catch( const std::exception& ex) {
        infile.close();
        std::cout << "Failed to read input file :"<< argv[1] <<  ex.what() << std::endl;
        return StatusCode::FATAL_ERROR;
    } 
    catch(...) {
        infile.close();
        return StatusCode::FATAL_ERROR;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();

    UrlContentGetter contentGetter;
    std::vector<std::string> htmls;
    for (int i = 0; i< static_cast<int>(urls.size()); ++i)
    {
        auto html = contentGetter.getUrlContent(urls[i]);
        // Copy into new structure for later analysis
        htmls.emplace_back(html);
    }
    int indx = 0;
    
    // Brute-force multi-threading
    // With the knowledge that the current parsing code in getCleanDomTree
    // is inefficient (and hence time-consuming) and assuming that html sizes 
    // are large enough , the best use of multi-threading is for the following 
    // loop. Additionally, writing the results atomically to an array will be 
    // less costly than mapping to separate arrays and then reducing.  At this
    // point this approach is the most maintainable with lower risks of errors.
    std::map<int, std::tuple<uint64_t, uint64_t, uint64_t>> urlStatMap;

    omp_set_num_threads(numThreadsRequested);
    
    #pragma omp parallel
    {
        #pragma omp for
        for (int i=0; i < htmls.size(); ++i) {
            // Pre-process the input HTML to remove special characters and other
            // cruft except for the interesting tag content.
            auto const& stats = getCleanDomTree(htmls[i]);

            #pragma omp critical
            {
                urlStatMap.insert({i, {std::get<0>(stats), 
                                std::get<1>(stats), 
                                std::get<2>(stats)} });
            }
        }
    }

    // Write out results in table
    std::cout << std::endl
              << std::setw(5) << "ID" << "\t" 
              << std::setw(40) << "URL" << "\t"
              << std::setw(15) << "# Nodes" << "\t"
              << std::setw(15) << "# Leaf Nodes" << "\t"
              << std::setw(15) << "# Div Nodes"
              << std::endl;
    uint64_t index = 0;
    for (auto const& pair : urlStatMap)
    {
        auto stats = pair.second;
        std::cout 
                << std::setw(5) << pair.first+1 << "\t" 
                << std::setw(40) << urls[pair.first].substr(0, std::min(40u, static_cast<unsigned int>(urls[indx].size()))) << "\t"
                << std::setw(15) << std::get<0>(stats) 
                << std::setw(15) << std::get<1>(stats)
                << std::setw(15) << std::get<2>(stats)
                << std::endl;
    }
    
    // Following code uses boost spirit, qi, and fusion libraries 
    // to construct an abstract syntax tree to do more detailed
    // parsing. However, the lack of complete grammar causes it to  
    // fail un-gracefully. No effort was spent to make correct the
    // root cause nor making it thread-safe. This initial implementation
    // is abandoned in favor of the simpler regex parser above.
    //  
    /*
     for (const auto& html : htmls)
    {
        const auto& html = htmls[i];
        std::tuple<unsigned int, unsigned int, unsigned int> stats;
        std::this_thread::sleep_for(5000ms);
        if (!html.empty())
           stats = parseHtml(html);
        std::cout << "Num nodes" << std::get<0>(stats)
            << "Num leaf nodes" << std::get<1>(stats)
            << "Num Div nodes" << std::get<2>(stats)
            << std::endl;
    }
    */
     
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed time "
              << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()/1000.
              << " seconds\n";
    return StatusCode::SUCCESS;
}
