/**
 * File: rss-index.cc
 * ------------------
 * Presents the implementation of the RSSIndex class, which is
 * little more than a glorified map.
 */

#include "rss-index.h"
#include "utils.h"
#include <algorithm>

using namespace std;

void RSSIndex::add(const Article& article, const vector<string>& words) {
  vector<string> sortedWords(words);
  sort(sortedWords.begin(),sortedWords.end());
  vector<string> intrsctn;
  string url = article.url;
  string server = getURLServer(article.url);
  string title = article.title;
  pair<string,string> duo = make_pair(title, server);
  if(tracker.find(duo) != tracker.end()){
        set_intersection(tracker[duo].second.cbegin(), tracker[duo].second.cend(), sortedWords.cbegin(), sortedWords.cend(), back_inserter(intrsctn));
        tracker[duo].second = intrsctn;
        if(url < tracker[duo].first) tracker[duo].first = url;
  }else {
        tracker[duo].second = sortedWords;
        tracker[duo].first = url;
   }
}

void RSSIndex::updateIndex(){
  for(auto& record : tracker){
      Article article;
      article.title = record.first.first; //retrieve title from tracker map key
      article.url = record.second.first;  //retrieve url from tracker map value
      for (const string& word : record.second.second) { // iteration via for keyword, yay C++11
          index[word][article]++;
      }
  }
}

static const vector<pair<Article, int> > emptyResult;
vector<pair<Article, int> > RSSIndex::getMatchingArticles(const string& word) const {
  auto indexFound = index.find(word);
  if (indexFound == index.end()) return emptyResult;
  const map<Article, int>& matches = indexFound->second;
  vector<pair<Article, int> > v;
  for (const pair<Article, int>& match: matches) v.push_back(match);
  sort(v.begin(), v.end(), [](const pair<Article, int>& one,
                              const pair<Article, int>& two) {
   return one.second > two.second || (one.second == two.second && one.first < two.first);
  });
  return v;
}

