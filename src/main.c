#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#define MAX_BOOKS 16

// Structure to store book details
typedef struct {
    char *url;
    char *cover_image;
    char *title;
    char *price;
} Book;

// Structure to hold the HTML response data
struct Memory {
    char *data;
    size_t size;
};

// Callback function to write fetched HTML data into memory
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc failed)\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;

    return real_size;
}

// Function to perform an HTTP GET request and fetch HTML content
struct Memory fetchHTML(CURL *curl_handle, const char *url) {
    CURLcode res;
    struct Memory chunk;

    chunk.data = malloc(1);
    chunk.size = 0;

    // Set URL and callback function for writing data
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36");

    // Perform the HTTP GET request
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to fetch URL: %s\n", curl_easy_strerror(res));
    }

    return chunk;
}

int main(void) {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();

    // Array to hold book details
    Book books[MAX_BOOKS];
    int bookCount = 0;

    // Fetch HTML content from the bookstore website
    struct Memory html_content = fetchHTML(curl_handle, "http://books.toscrape.com/");

    // Parse the fetched HTML content
    htmlDocPtr doc = htmlReadMemory(html_content.data, (unsigned long)html_content.size, NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_RECOVER);
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

    // XPath query to find book elements on the page
    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *)"//article[@class='product_pod']", xpathCtx);

    // Loop through each book element and extract details
    for (int i = 0; i < result->nodesetval->nodeNr && bookCount < MAX_BOOKS; ++i) {
        xmlNodePtr bookNode = result->nodesetval->nodeTab[i];
        xmlXPathSetContextNode(bookNode, xpathCtx);

        // Extract URL
        xmlNodePtr urlNode = xmlXPathEvalExpression((xmlChar *)".//h3/a", xpathCtx)->nodesetval->nodeTab[0];
        char *url = (char *)xmlGetProp(urlNode, (xmlChar *)"href");

        // Extract cover image
        xmlNodePtr coverNode = xmlXPathEvalExpression((xmlChar *)".//div[@class='image_container']/a/img", xpathCtx)->nodesetval->nodeTab[0];
        char *cover_image = (char *)xmlGetProp(coverNode, (xmlChar *)"src");

        // Extract title
        xmlNodePtr titleNode = xmlXPathEvalExpression((xmlChar *)".//h3/a", xpathCtx)->nodesetval->nodeTab[0];
        char *title = (char *)xmlGetProp(titleNode, (xmlChar *)"title");

        // Extract price
        xmlNodePtr priceNode = xmlXPathEvalExpression((xmlChar *)".//div[@class='product_price']/p[@class='price_color']", xpathCtx)->nodesetval->nodeTab[0];
        char *price = (char *)xmlNodeGetContent(priceNode);

        // Store the extracted details in a Book structure
        Book book;
        book.url = strdup(url);
        book.cover_image = strdup(cover_image);
        book.title = strdup(title);
        book.price = strdup(price);

        // Add the book to the array
        books[bookCount++] = book;
    }

    // Clean up resources
    free(html_content.data);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    // Open a CSV file to save the scraped data
    FILE *file = fopen("books.csv", "w");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Write CSV header
    fprintf(file, "URL,Cover Image,Title,Price\n");

    // Write each book's details to the CSV file
    for (int i = 0; i < bookCount; i++) {
        fprintf(file, "%s,%s,%s,%s\n", books[i].url, books[i].cover_image, books[i].title, books[i].price);
    }

    fclose(file);

    // Free dynamically allocated memory for each book
    for (int i = 0; i < bookCount; i++) {
        free(books[i].url);
        free(books[i].cover_image);
        free(books[i].title);
        free(books[i].price);
    }

    return 0;
}
