import sys
import socket
import urllib
import sgmllib
from urlparse import urlparse
import os

download_home_page = "http://lk8000.it/lkmaps/mapindex.html"

visited_pages = []

class link_parser(sgmllib.SGMLParser):
    hyperlinks = []
        
    def parse(self, s):
        self.feed(s)
        self.close()
        return self

    def start_a(self, attributes):
        for name, value in attributes:
            if name == "href":
                self.hyperlinks.append(value)

def open_file(url):
    print "Downloading file " + url + " ..."
    socket.setdefaulttimeout(10)
    try:
        return urllib.urlopen(url)
    except IOError:
        print "Download of file " + url + " failed!"
        return None
    
def download_file(url, local_dir):
    print "Downloading file " + url + " ..."
    
    filename = os.path.basename(urlparse(url).path)
    local = os.path.join(local_dir, filename)
    
    if os.path.exists(local):
        return
    
    socket.setdefaulttimeout(10)
    try:
        urllib.urlretrieve(url, local)
    except IOError:
        print "Download of file " + url + " failed!"
    
def recursive_download(url, folder):
    if url in visited_pages:
        return
    
    visited_pages.append(url)
    
    f = open_file(url)
    if f == None:
        return
    
    parser = link_parser().parse(f.read())
    
    for link in parser.hyperlinks:
        # Download files
        if link.upper().endswith(".LKM"):
            download_file(link, folder)

        if link.upper().endswith(".HTM") or link.upper().endswith(".HTML"):
            if link.lower().startswith("/lkmaps"):
                link = "http://www.lk8000.it" + link

            if link.lower().startswith("http://www.lk8000.it/lkmaps"):
                recursive_download(link, folder)

def main():
    if len(sys.argv) < 2:
        print "Too few arguments given! Assuming \"../data/lkm/\""
        folder = "../data/lkm/"
    else:    
        folder = sys.argv[1]
        
    if not os.path.exists(folder) and os.path.isdir(folder):
        print "Download folder \"" + file + "\" does not exist!"
        return
    
    recursive_download(download_home_page, folder) 
    
if __name__ == '__main__':
    main()    
