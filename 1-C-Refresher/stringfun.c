// Ben Kuoch 2028
// P.S. I did the extra credit command for this assignment!

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
//add additional prototypes here
void reverse_string(char *buff, int len, int actual_str_len);
void print_words(char *, int);
int replace_string(char *buff, int len, const char *old_word, const char *new_word);


int setup_buff(char *buff, char *user_str, int len) {
    char *src = user_str;
    char *dst = buff;
    int char_count = 0;

    while (*src && char_count < len) {
        if (*src != ' ' && *src != '\t') {
            *dst++ = *src;
            char_count++;
        } else if (dst != buff && *(dst - 1) != ' ') {
            *dst++ = ' ';
            char_count++;
        }
        src++;
    }

    if (*src) return -1;  

    while (char_count < len) {
        *dst++ = '.';
        char_count++;
    }

    return dst - buff;  
}


void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other_options]\n", exename);
    printf("\nwhere:\n  -h    prints help about the program\n  -c    counts of the number of words in the 'sample string'\n  -r    reverses the characters (in place) in 'sample string'\n  -w    prints the individual words and their length in the 'sample string'\n  -x    takes sample string and 2 other strings, replaces the first with second" );
    

}

int count_words(char *buff, int len, int str_len){
    //YOU MUST IMPLEMENT
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (*(buff + i) != ' ') {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
    }

    return count;

}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS
void reverse_string(char *buff, int len, int actual_str_len) {
    char *start = buff;
    char *end = buff + actual_str_len - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

void print_words(char *buff, int len) {
    printf("Word Print\n");
    printf("----------\n");

    int word_count = 1;
    int i = 0;
    while (i < len && *(buff + i) != '\0') {
        if (*(buff + i) != ' ' && *(buff + i) != '.') {  //periods ignored
            char *start = buff + i;
            while (*(buff + i) != ' ' && *(buff + i) != '\0' && *(buff + i) != '.') {
                i++;
            }
            int word_len = i - (start - buff);
            printf("%d. ", word_count++);
            for (char *c = start; c < buff + i; c++) {
                putchar(*c);
            }
            printf(" (%d)\n", word_len);
        } else {
            i++;
        }
    }
}

int replace_string(char *buff, int len, const char *old_word, const char *new_word) {
    int old_len = strlen(old_word);
    int new_len = strlen(new_word);

    char *pos = strstr(buff, old_word);
    if (!pos) {
        return 0;
    }

    // Check if the replacement fits within the buffer
    int diff = new_len - old_len;
    int remaining_len = len - (pos - buff) - old_len;

    if (diff > 0 && remaining_len < diff) {
        printf("Error: Buffer overflow while replacing word.\n");
        return -1;
    }

    // do actually replacement
    memmove(pos + new_len, pos + old_len, remaining_len);
    memcpy(pos, new_word, new_len); 

    return 1; 
}



int main(int argc, char *argv[]) {
    char *buff;            // Placeholder for the internal buffer
    char *input_string;    // Holds the string provided by the user on cmd line
    char opt;              // Used to capture user option from cmd line
    int rc;                // Used for return codes
    int user_str_len;      // Length of user-supplied string

    int actual_str_len;    // Actual length of the string (without padding)


    // TODO: #1: Why is this safe? 
    // This is safe because if argv[1] does not exist, argc < 2 protects against accessing it.
    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); // Get the option flag

    // Handle the help flag and then exit normally
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    // TODO: #2: Document the purpose of the if statement below.
    // The purpose of the if statement is to make sure that enough arguments are provided.
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; // Capture the user input string

    // TODO: #3: Allocate space for the buffer using malloc
    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(99);
    }

    actual_str_len = strlen(input_string);

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error counting words, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;

        case 'r':
            reverse_string(buff, BUFFER_SZ, actual_str_len);
            printf("Reversed String: ");
            // Print only the actual string length (excluding padding)
            for (int i = 0; i < actual_str_len; i++) {
                putchar(*(buff + i));
            }
            putchar('\n');
            break;

        case 'w':
            print_words(buff, user_str_len);
            break;


        case 'x': {
            if (argc < 5) {
                usage(argv[0]);
                free(buff);
                exit(1);
            }
            char *old_word = argv[3];
            char *new_word = argv[4];

            rc = replace_string(buff, BUFFER_SZ, old_word, new_word);
            if (rc < 0) {
                free(buff);
                exit(2);
            }
            printf("Modified String: ");
            for (int i = 0; i < BUFFER_SZ; i++) {
                if (buff[i] == '.') break;  
                putchar(buff[i]);
            }
            putchar('\n');
            break;
        }


        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    // TODO: #6: Free buffer before exiting
    //print_buff(buff, BUFFER_SZ);   //for printing buffer if wanted
    free(buff);
    exit(0);
}


//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          Passing both the pointer and the length is good practice because it makes sure that the functions don't go past the buffer's boundaries which could cause overflow.

