//--------------------------------------------------------------------------------------------------
// System Programming                         I/O Lab                                    Fall 2020
//
/// @file
/// @brief resursively traverse directory tree and list all entries
/// @author Jinsol Park
/// @studid 2018-14000
//--------------------------------------------------------------------------------------------------

// for_fin_sub_real

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>

#define MAX_DIR 64            ///< maximum number of directories supported

/// @brief output control flags
#define F_TREE      0x1       ///< enable tree view
#define F_SUMMARY   0x2       ///< enable summary
#define F_VERBOSE   0x4       ///< turn on verbose mode

/// @brief struct holding the summary
struct summary {
  unsigned int dirs;          ///< number of directories encountered
  unsigned int files;         ///< number of files
  unsigned int links;         ///< number of links
  unsigned int fifos;         ///< number of pipes
  unsigned int socks;         ///< number of sockets

  unsigned long long size;    ///< total size (in bytes)
  unsigned long long blocks;  ///< total number of blocks (512 byte blocks)
};


/// @brief abort the program with EXIT_FAILURE and an optional error message
///
/// @param msg optional error message or NULL
void panic(const char *msg)
{
  if (msg) fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}


/// @brief read next directory entry from open directory 'dir'. Ignores '.' and '..' entries
///
/// @param dir open DIR* stream
/// @retval entry on success
/// @retval NULL on error or if there are no more entries
struct dirent *getNext(DIR *dir)
{
  struct dirent *next;
  int ignore;

  do {
    errno = 0;
    next = readdir(dir);
    if (errno != 0) perror(NULL);
    ignore = next && ((strcmp(next->d_name, ".") == 0) || (strcmp(next->d_name, "..") == 0));
  } while (next && ignore);

  return next;
}


/// @brief qsort comparator to sort directory entries. Sorted by name, directories first.
///
/// @param a pointer to first entry
/// @param b pointer to second entry
/// @retval -1 if a<b
/// @retval 0  if a==b
/// @retval 1  if a>b
static int dirent_compare(const void *a, const void *b)
{
  struct dirent *e1 = *(struct dirent**)a;
  struct dirent *e2 = *(struct dirent**)b;

  // if one of the entries is a directory, it comes first
  if (e1->d_type != e2->d_type) {
    if (e1->d_type == DT_DIR) return -1;
    if (e2->d_type == DT_DIR) return 1;
  }

  // otherwise sorty by name
  return strcmp(e1->d_name, e2->d_name);
}



/// @brief recursively process directory @a dn and print its tree
///
/// @param dn absolute or relative path string
/// @param pstr prefix string printed in front of each entry
/// @param stats pointer to statistics
/// @param flags output control flags (F_*)
void processDir(const char *dn, const char *pstr, struct summary *stats, unsigned int flags)
{
  int path_len = strlen(dn);
  
  DIR* for_count = opendir(dn);               // open directory for counting number of entries
  if(for_count == NULL) {panic("No such file or directory.\n Process terminates.\n");}
  int cnt = 0;          
  struct dirent* d;                           // a temporary dirent pointer
  while((d = getNext(for_count)) != NULL){    // until getNext returns NULL, which means that there are no more entries to read
    cnt += 1;                                 // increase the number of counts (entries in directory)
  }
  closedir(for_count); 

  DIR* dir = opendir(dn);                                                             
  struct dirent** dir_arr = (struct dirent**) malloc(sizeof(struct dirent*) * cnt);   // make dir_arr as a dirent** type
  if(dir_arr == NULL) panic("ERROR: Out of memory\n");
  struct dirent dir_cpy[cnt];                                                         // a dir_cpy array to keep on stack

  for(int i = 0 ; i < cnt ; i++){
    dir_arr[i] = getNext(dir);      // put the entries in dir to dir_arr in order
  }
  closedir(dir); 

  

  qsort(dir_arr, cnt, sizeof(dir_arr[0]), dirent_compare);  // qsort with given comparator

  for(int i = 0 ; i < cnt ; i++){
    dir_cpy[i] = *dir_arr[i];
  }                                                         // put the dirent entries on stack(dir_cpy) for less confusion of using pointers

  for(int i = 0 ; i < cnt ; i++){
    if(dir_cpy[i].d_type == DT_DIR) {stats->dirs++;}          // update stats->dirs if dirent d_type is DIR
    else if(dir_cpy[i].d_type == DT_FIFO) {stats->fifos++; }  // update stats->fifos if dirent d_type is FIFO
    else if(dir_cpy[i].d_type == DT_REG) {stats->files++;}    // update stats->files if dirent d_type is REG
    else if(dir_cpy[i].d_type == DT_LNK) {stats->links++;}    // update stats->links if dirent d_type is LNK
    else if(dir_cpy[i].d_type == DT_SOCK) {stats->socks++; }  // update stats->socks if dirent d_type is SOCK
    
    struct stat fileinfo;
    int fd_length = strlen(dn) + strlen(dir_cpy[i].d_name) + 2; // fd length is assigned to be the length of newly created subpath
    char* fd = (char*) malloc(fd_length);
    if(fd == NULL) panic("ERROR: Out of memory\n");
    strcpy(fd, dn);                                             // fd = dn (ex. demo/)
    if(dn[path_len-1] != '/') strcat(fd, "/");                  // adding / at enf if not have / already, make fd as new path
    strcat(fd, dir_cpy[i].d_name);                              // (ex. demo/subdir1)
    lstat(fd, &fileinfo);                                       // call lstat to put the information of diven subdir/file into fileinfo
    

    unsigned long long sz = fileinfo.st_size;
    stats->size += sz;                                          // update stats->size
    stats->blocks += fileinfo.st_blocks;                        // update stats->blocks

    printf("%s", pstr);                 // first, print the pstr
    if(flags & F_TREE){                 // if flags is -t
      if(i != cnt-1){ printf("|-");}    // if this is not the last entry, print |-
      else{ printf("`-");}              // if this is the last entry
    }
    else{ printf("  ");}                // if not -t mode, just print double space
    
    if(!(fileinfo.st_mode & S_IRUSR)) {printf("ERROR: Permission denied\n"); continue;} // checking for file permission

    int actual_name_start = strlen(pstr) + 2;
    int space_for_name = 54 - actual_name_start;      // finding out remaining space after name
    int name_length = strlen(dir_cpy[i].d_name);      // the length of name that needs to be printed
    if(flags & F_VERBOSE){                            // if verbose mode
      if(space_for_name < name_length){               // if name is too long to fit within given area
        for(int k = 0 ; k < space_for_name-3 ; k++){  //print the name, skipping the last three characters
          printf("%c", dir_cpy[i].d_name[k]);
        }
        printf("...");                                                          // in the remaining area fill with ...
      }else{                                                                    // if name does fit in given area
        printf("%s", dir_cpy[i].d_name);                                        // just print the name
        for(int k = 0 ; k < space_for_name - name_length ; k++){printf(" ");}   // then print space for the remaining area
      }
    }else printf("%s\n", dir_cpy[i].d_name);          // if not verbose mode, just print the file/dir name and newline
      
    if(flags & F_VERBOSE){                            // need to print additional information when verbose mode
      printf("  ");       

      char *user = (char*) malloc(9); 
      if(user == NULL) panic("ERROR: Out of memory\n");                    // if malloc returns 0, it means that malloc failed
      user = getpwuid(fileinfo.st_uid)->pw_name;                        // getting user name from getpwuid
      int username_length = strlen(user);
      if(username_length < 8){                                          // if username is shorter than given space
        for(int k = 0 ; k < 8 - username_length ; k++){ printf(" ");}   // print space beforehand
        printf("%s", user);
      }else{                                                            // if username fits in the given space
        for(int k = 0 ; k < 8 ; k++){ printf("%c", user[k]);}           // print char by char from the beginning, incase of overflow
      }

      printf(":");
     
      char *group = (char*) malloc(9);
      if(group == 0) panic("ERROR: Out of memory\n");
      group = getgrgid(fileinfo.st_gid)->gr_name;                       // getting group name from getgrgid
      int groupname_length = strlen(group);
      if(groupname_length < 8){                                         // if group name is shorter than given space
        printf("%s", group);                                            // just print the group name
        for(int k = 0 ; k < 8-groupname_length ; k++){ printf(" ");}    // print space for remining area
      }else{                                                            // if group name is longer than given area
        for(int k = 0 ; k < 8 ; k++){ printf("%c", group[k]);}          // print char by char in case of overflow
      }

      printf("  ");

      char s[11];
      sprintf(s, "%llu", sz);                                           // make the size into a string
      int size_length = strlen(s);
      if(size_length < 10){                                             // if length of size string is shorter than given area
        for(int k = 0 ; k < 10-size_length ; k++){ printf(" ");}        // print space beforehand
      }
      printf("%s", s);
      

      printf("  ");

      int num_blocks = fileinfo.st_blocks;                              // get num_blocks from fileinfo
      char b[9];
      sprintf(b, "%d", num_blocks);                                     // make num_blocks into a string
      int num_blocks_length = strlen(b);
      if(num_blocks_length < 8){                                        // if length of num_blocks is shorter than given area
        for(int k = 0 ; k < 8-num_blocks_length ; k++){ printf(" ");}   // print space beforehand
      }
      printf("%s", b);

      printf("  ");

      if(S_ISREG(fileinfo.st_mode)) { printf(" \n");}         // print nothing if file type
      else if(S_ISDIR(fileinfo.st_mode)) {printf("d\n");}     // print d if directory
      else if(S_ISLNK(fileinfo.st_mode)) {printf("l\n");}     // print l if link
      else if(S_ISCHR(fileinfo.st_mode)) {printf("c\n");}     // print c if character
      else if(S_ISBLK(fileinfo.st_mode)) {printf("b\n");}     // print b if block
      else if(S_ISFIFO(fileinfo.st_mode)) {printf("f\n");}    // print f if fifo
      else if(S_ISSOCK(fileinfo.st_mode)) {printf("s\n");}    // orint s if socket
    }
    if(dir_cpy[i].d_type == DT_DIR){
      char* new_pstr = (char*) malloc((strlen(pstr)+3));                      // make new pstr
      if(new_pstr == NULL) panic("ERROR: Out of memory\n");
      strcpy(new_pstr,pstr);                                                  // copy this into new pstr
      if(flags & F_TREE){
        if(cnt != 1) {processDir(fd, strcat(new_pstr, "| "), stats, flags);}  // if not the last entry, new path(fd) and newpstr+|-
        else {processDir(fd, strcat(new_pstr, "  "), stats, flags);}          // if last entry, new path(fd) and newpstr+doublespace
      }
      else{processDir(fd, strcat(new_pstr, "  "), stats, flags);}             // if not -t mode
      free(new_pstr);
    }
  }
  free(dir_arr);
}


/// @brief print program syntax and an optional error message. Aborts the program with EXIT_FAILURE
///
/// @param argv0 command line argument 0 (executable)
/// @param error optional error (format) string (printf format) or NULL
/// @param ... parameter to the error format string
void syntax(const char *argv0, const char *error, ...)
{
  if (error) {
    va_list ap;

    va_start(ap, error);
    vfprintf(stderr, error, ap);
    va_end(ap);

    printf("\n\n");
  }

  assert(argv0 != NULL);

  fprintf(stderr, "Usage %s [-t] [-s] [-v] [-h] [path...]\n"
                  "Gather information about directory trees. If no path is given, the current directory\n"
                  "is analyzed.\n"
                  "\n"
                  "Options:\n"
                  " -t        print the directory tree (default if no other option specified)\n"
                  " -s        print summary of directories (total number of files, total file size, etc)\n"
                  " -v        print detailed information for each file. Turns on tree view.\n"
                  " -h        print this help\n"
                  " path...   list of space-separated paths (max %d). Default is the current directory.\n",
                  basename(argv0), MAX_DIR);

  exit(EXIT_FAILURE);
}


/// @brief program entry point
int main(int argc, char *argv[])
{
  //
  // default directory is the current directory (".")
  //
  const char CURDIR[] = ".";
  const char *directories[MAX_DIR];
  int   ndir = 0;

  struct summary dstat, tstat;
  unsigned int flags = 0;

  //
  // parse arguments
  //
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // format: "-<flag>"
      if      (!strcmp(argv[i], "-t")) flags |= F_TREE;
      else if (!strcmp(argv[i], "-s")) flags |= F_SUMMARY;
      else if (!strcmp(argv[i], "-v")) flags |= F_VERBOSE;
      else if (!strcmp(argv[i], "-h")) syntax(argv[0], NULL);
      else syntax(argv[0], "Unrecognized option '%s'.", argv[i]);
    } else {
      // anything else is recognized as a directory
      if (ndir < MAX_DIR) {
        directories[ndir++] = argv[i];
      } else {
        printf("Warning: maximum number of directories exceeded, ignoring '%s'.\n", argv[i]);
      }
    }
  }

  // if no directory was specified, use the current directory
  if (ndir == 0) directories[ndir++] = CURDIR;


  //
  // process each directory
  //
  // TODO
  tstat.blocks = 0;
  tstat.dirs = 0;
  tstat.fifos = 0;
  tstat.files = 0;
  tstat.links = 0;
  tstat.size = 0;
  tstat.socks = 0;  // initialize tstat members to 0

  for(int i = 0 ; i < ndir ; i++){

    if(flags & F_SUMMARY){                            // if summary, print header
      printf("Name");                                 
      if(flags & F_VERBOSE){                          // if verbose, need to print additional stuff
        for(int k = 0 ; k < 56 ; k++) {printf(" ");}  
        printf("User:Group");                        
        for(int k = 0 ; k < 11 ; k++) {printf(" ");}
        printf("Size");
        for(int k = 0 ; k < 4 ; k++) {printf(" ");}
        printf("Blocks Type\n");
      }else printf("\n");                             // if not verbose, just printing newline after Name
      for(int k = 0 ; k < 100 ; k++) {printf("-");}   // printing header line
      printf("\n");
    }

    dstat.blocks = 0;
    dstat.dirs = 0;
    dstat.fifos = 0;
    dstat.files = 0;
    dstat.links = 0;
    dstat.size = 0;
    dstat.socks = 0;                                  // initialize dstat members to 0

    printf("%s\n", directories[i]);                   // first, print directory name
    processDir(directories[i], "", &dstat, flags);    // call processdir with no pstr, given flags

    tstat.blocks += dstat.blocks;   
    tstat.dirs += dstat.dirs;
    tstat.fifos += dstat.fifos;
    tstat.files += dstat.files;
    tstat.links += dstat.links;
    tstat.size += dstat.size;
    tstat.socks += dstat.socks;                           // update tstat by using dstat results from processing current directory
    
    if(flags & F_SUMMARY){                                // if summary, print footer
      for(int k = 0 ; k < 100 ; k++) {printf("-");}       // print footer line
      printf("\n");
      char* sum = (char*) malloc(102);                    // string buffer for final print
      if(sum == NULL) panic("ERROR: Out of memory\n");
      char* tmp = (char*) malloc(70);                     // temporary string buffer
      if(tmp == NULL) panic("ERROR: Out of memory\n");

      if(dstat.files == 1) {sprintf(sum,"%d file, ", 1);} // if number of files is 1
      else {sprintf(sum, "%d files, ", dstat.files);}     // if number of files is not 1

      if(dstat.dirs == 1) {strcat(sum, "1 directory, ");} 
      else {sprintf(tmp, "%d directories, ", dstat.dirs); strcat(sum, tmp);}    // if #dirs is not 1, format string, and concat to sum

      if(dstat.links == 1) {strcat(sum, "1 link, ");}
      else {sprintf(tmp, "%d links, ", dstat.links); strcat(sum, tmp);}         // if #links is not 1, format string, and concat to sum

      if(dstat.fifos == 1) {strcat(sum, "1 pipe, ");}
      else {sprintf(tmp, "%d pipes, ", dstat.fifos); strcat(sum, tmp);}         // if #fifos is not 1, format string, and concat to sum

      if(dstat.socks == 1) {strcat(sum, "and 1 socket");}
      else {sprintf(tmp, "and %d sockets", dstat.socks); strcat(sum, tmp);}     // if #sockets is not 1, format string, and concat to sum
      
      if(flags & F_VERBOSE){                                // when -v option nis also on, need to print more stuff at footer
        sprintf(tmp, "%llu", dstat.size);                   // make dstat.size into a string
        int left = 85-strlen(sum)-strlen(tmp);              // find remaining space from format
        for(int k = 0 ; k < left ; k++){strcat(sum, " ");}
        strcat(sum, tmp);                                   // concatenate size onto the sum

        strcat(sum, " ");

        sprintf(tmp, "%lld", dstat.blocks);                 // make dstat.blocks into a string
        int tmp_len = strlen(tmp);
        for(int k = 0 ; k < 9-tmp_len ; k++) {strcat(sum, " ");}  // find remaining space from format
        strcat(sum, tmp);                                   // concatenate size onto the sum
        strcat(sum, "\n");
      }
      printf("%s\n", sum);                                  // print the final line
      free(sum);
      free(tmp);
    }
    printf("\n");
  }
  //
  // print grand total
  //
  if ((flags & F_SUMMARY) && (ndir > 1)) {
    printf("Analyzed %d directories:\n"
           "  total # of files:        %16d\n"
           "  total # of directories:  %16d\n"
           "  total # of links:        %16d\n"
           "  total # of pipes:        %16d\n"
           "  total # of sockets:      %16d\n",
           ndir, tstat.files, tstat.dirs, tstat.links, tstat.fifos, tstat.socks);

    if (flags & F_VERBOSE) {
      printf("  total file size:         %16llu\n"
             "  total # of blocks:       %16llu\n",
             tstat.size, tstat.blocks);
    }

  }

  //
  // that's all, folks
  //
  return EXIT_SUCCESS;
}
