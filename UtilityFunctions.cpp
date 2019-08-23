/* Utility functions usually for debugging
 */


#include "UtilityFunctions.hpp"
#include <stdio.h>
#include <sys/stat.h>
#include <string>




/*Spinning cursors*/

void advance_cursor() {
  static int pos=0;
  char cursor[4]={'/','-','\\','|'};
  printf("%c\b", cursor[pos]);
  fflush(stdout);
  pos = (pos+1) % 4;
}




/**
 * Get the size of a file.
 * @param filename The name of the file to check size for
 * @return The filesize, or 0 if the file does not exist.
 */
size_t getFilesize(const std::string& filename) {
    struct stat st;
    if(stat(filename.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_size;
}

/**
 * Check if file exists.
 * @param filename The name of the file to check size for
 * @return True if it does exist, False if it doesnt
 */

bool doesFileExist (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


