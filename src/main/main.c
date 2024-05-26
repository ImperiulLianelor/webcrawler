#include "curl/curl.h" 
#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"
#include <string.h>

#define MAX_PRODUCTS 16
    
    // initialize a data structure to
    // store the scraped data
    typedef struct
    {
        char *url;
        char *image;
        char *name;
        char *price;
    } PokemonProduct;
    
    struct CURLResponse
    {
        char *html;
        size_t size;
    };
    
    static size_t
    WriteHTMLCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        size_t realsize = size * nmemb;
        struct CURLResponse *mem = (struct CURLResponse *)userp;
      
        char *ptr = realloc(mem->html, mem->size + realsize + 1);
        if (!ptr)
        {
            printf("Not enough memory available (realloc returned NULL)\n");
            return 0;
        }
      
        mem->html = ptr;
        memcpy(&(mem->html[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->html[mem->size] = 0;
      
        return realsize;
    }
    
    struct CURLResponse GetRequest(CURL *curl_handle, const char *url)
    {
        CURLcode res;
        struct CURLResponse response;
      
        // initialize the response
        response.html = malloc(1);
        response.size = 0;
      
        // specify URL to GET
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
      
        // send all data returned by the server to WriteHTMLCallback
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteHTMLCallback);
      
        // pass "response" to the callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
      
        // set a User-Agent header
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");
      
        // perform the GET request
        res = curl_easy_perform(curl_handle);
      
        // check for HTTP errors
        if (res != CURLE_OK)
        {
            fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(res));
        }
      
        return response;
    }
    
    int main(void)
    {
        // initialize curl globally
        curl_global_init(CURL_GLOBAL_ALL);
      
        // initialize a CURL instance
        CURL *curl_handle = curl_easy_init();
      
        // initialize the array that will contain
        // the scraped data
        PokemonProduct products[MAX_PRODUCTS];
        int productCount = 0;
      
        // get the HTML document associated with the page
        struct CURLResponse response = GetRequest(curl_handle, "https://scrapeme.live/shop/");
    
        // parse the HTML document returned by the server
        htmlDocPtr doc = htmlReadMemory(response.html, (unsigned long)response.size, NULL, NULL, HTML_PARSE_NOERROR);
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
    
        // get the product HTML elements on the page
        xmlXPathObjectPtr productHTMLElements = xmlXPathEvalExpression((xmlChar *)"//li[contains(@class, 'product')]", context);
    
        // iterate over them and scrape data from each of them
        for (int i = 0; i < productHTMLElements->nodesetval->nodeNr; ++i)
        {
            // get the current element of the loop
            xmlNodePtr productHTMLElement = productHTMLElements->nodesetval->nodeTab[i];
      
            // set the context to restrict XPath selectors
            // to the children of the current element
            xmlXPathSetContextNode(productHTMLElement, context);
            xmlNodePtr urlHTMLElement = xmlXPathEvalExpression((xmlChar *)".//a", context)->nodesetval->nodeTab[0];
            char *url = (char *)xmlGetProp(urlHTMLElement, (xmlChar *)"href");
            xmlNodePtr imageHTMLElement = xmlXPathEvalExpression((xmlChar *)".//a/img", context)->nodesetval->nodeTab[0];
            char *image = (char *)(xmlGetProp(imageHTMLElement, (xmlChar *)"src"));
            xmlNodePtr nameHTMLElement = xmlXPathEvalExpression((xmlChar *)".//a/h2", context)->nodesetval->nodeTab[0];
            char *name = (char *)(xmlNodeGetContent(nameHTMLElement));
            xmlNodePtr priceHTMLElement = xmlXPathEvalExpression((xmlChar *)".//a/span", context)->nodesetval->nodeTab[0];
            char *price = (char *)(xmlNodeGetContent(priceHTMLElement));
      
            // store the scraped data in a PokemonProduct instance
            PokemonProduct product;
            product.url = strdup(url);
            product.image = strdup(image);
            product.name = strdup(name);
            product.price = strdup(price);
      
            // free up the resources you no longer need
            free(url);
            free(image);
            free(name);
            free(price);
      
            // add a new product to the array
            products[productCount] = product;
            productCount++;
        }
    
        // free up the allocated resources
        free(response.html);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        xmlCleanupParser();
      
        // cleanup the curl instance
        curl_easy_cleanup(curl_handle);
        // cleanup the curl resources
        curl_global_cleanup();
      
        // open a CSV file for writing
        FILE *csvFile = fopen("products.csv", "w");
        if (csvFile == NULL)
        {
            perror("Failed to open the CSV output file!");
            return 1;
        }
        // write the CSV header
        fprintf(csvFile, "url,image,name,price\n");
        // write each product's data to the CSV file
        for (int i = 0; i < productCount; i++)
        {
            fprintf(csvFile, "%s,%s,%s,%s\n", products[i].url, products[i].image, products[i].name, products[i].price);
        }
      
        // close the CSV file
        fclose(csvFile);
      
        // free the resources associated with each product
        for (int i = 0; i < productCount; i++)
        {
            free(products[i].url);
            free(products[i].image);
            free(products[i].name);
            free(products[i].price);
        }
      
        return 0;
    }
