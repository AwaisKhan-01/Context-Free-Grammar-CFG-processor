#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Structure for a production
typedef struct {
    char* lhs;        // Left-hand side non-terminal
    char** rhs;       // Array of right-hand side alternatives
    int rhs_count;    // Number of alternatives
} Production;

// Helper Functions

// Trim leading and trailing whitespace
char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

// Split a string by a delimiter and trim each part
char** split_string(char* str, const char* delimiter, int* count) {
    char** parts = malloc(10 * sizeof(char*));
    int capacity = 10;
    int i = 0;
    char* copy = strdup(str);
    char* token = strtok(copy, delimiter);
    while (token != NULL) {
        if (i >= capacity) {
            capacity *= 2;
            parts = realloc(parts, capacity * sizeof(char*));
        }
        parts[i++] = strdup(trim(token));
        token = strtok(NULL, delimiter);
    }
    *count = i;
    free(copy);
    return parts;
}

// Split an alternative into symbols (by spaces)
char** split_alternative(char* alt, int* count) {
    return split_string(alt, " ", count);
}

// Join symbols into a string with spaces
char* join_symbols(char** symbols, int start, int end) {
    if (start >= end) return strdup("");
    int len = 0;
    for (int i = start; i < end; i++) len += strlen(symbols[i]) + 1;
    char* result = malloc(len);
    result[0] = '\0';
    for (int i = start; i < end; i++) {
        strcat(result, symbols[i]);
        if (i < end - 1) strcat(result, " ");
    }
    return result;
}

// Check if a non-terminal is already used
int is_used(char** used_nt, int used_count, char* name) {
    for (int i = 0; i < used_count; i++) {
        if (strcmp(used_nt[i], name) == 0) return 1;
    }
    return 0;
}

// Generate a unique new non-terminal (e.g., A', A'', etc.)
char* generate_new_nt(char* base, char** used_nt, int* used_count) {
    char* candidate = malloc(strlen(base) + 2);
    sprintf(candidate, "%s'", base);
    int primes = 1;
    while (is_used(used_nt, *used_count, candidate)) {
        free(candidate);
        candidate = malloc(strlen(base) + primes + 2);
        sprintf(candidate, "%s", base);
        for (int i = 0; i <= primes; i++) strcat(candidate, "'");
        primes++;
    }
    used_nt[*used_count] = strdup(candidate);
    (*used_count)++;
    return candidate;
}

// Simple Queue for non-terminals to process
typedef struct {
    char** nt_list;
    int size;
    int capacity;
} Queue;

void enqueue(Queue* q, char* nt) {
    if (q->size == q->capacity) {
        q->capacity *= 2;
        q->nt_list = realloc(q->nt_list, q->capacity * sizeof(char*));
    }
    q->nt_list[q->size++] = strdup(nt);
}

char* dequeue(Queue* q) {
    if (q->size == 0) return NULL;
    char* nt = q->nt_list[0];
    for (int i = 0; i < q->size - 1; i++) {
        q->nt_list[i] = q->nt_list[i + 1];
    }
    q->size--;
    return nt;
}

// Parse grammar from input.txt
Production* parse_grammar(const char* filename, int* prod_count, char*** used_nt, int* used_count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error opening %s\n", filename);
        exit(1);
    }
    char line[256];
    Production* productions = malloc(100 * sizeof(Production));
    int capacity = 100;
    *prod_count = 0;
    *used_nt = malloc(100 * sizeof(char*));
    int nt_capacity = 100;
    *used_count = 0;
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);
        if (strlen(trimmed) == 0) continue;
        char* arrow = strstr(trimmed, "->");
        if (arrow) {
            *arrow = '\0';
            char* lhs = strdup(trim(trimmed));
            char* rhs_str = trim(arrow + 2);
            int rhs_count;
            char** rhs = split_string(rhs_str, "|", &rhs_count);
            if (*prod_count >= capacity) {
                capacity *= 2;
                productions = realloc(productions, capacity * sizeof(Production));
            }
            productions[*prod_count].lhs = lhs;
            productions[*prod_count].rhs = rhs;
            productions[*prod_count].rhs_count = rhs_count;
            (*prod_count)++;
            if (!is_used(*used_nt, *used_count, lhs)) {
                if (*used_count >= nt_capacity) {
                    nt_capacity *= 2;
                    *used_nt = realloc(*used_nt, nt_capacity * sizeof(char*));
                }
                (*used_nt)[*used_count] = strdup(lhs);
                (*used_count)++;
            }
        }
    }
    fclose(file);
    return productions;
}

// Print the grammar
void print_grammar(Production* productions, int prod_count) {
    for (int i = 0; i < prod_count; i++) {
        printf("%s -> ", productions[i].lhs);
        for (int j = 0; j < productions[i].rhs_count; j++) {
            printf("%s", productions[i].rhs[j]);
            if (j < productions[i].rhs_count - 1) printf(" | ");
        }
        printf("\n");
    }
}

// Perform left factoring
void left_factoring(Production** productions, int* prod_count, char** used_nt, int* used_count) {
    Queue q = {malloc(100 * sizeof(char*)), 0, 100};
    for (int i = 0; i < *prod_count; i++) {
        enqueue(&q, (*productions)[i].lhs);
    }
    while (q.size > 0) {
        char* A = dequeue(&q);
        int idx = -1;
        for (int i = 0; i < *prod_count; i++) {
            if (strcmp((*productions)[i].lhs, A) == 0) {
                idx = i;
                break;
            }
        }
        if (idx == -1) {
            free(A);
            continue;
        }
        Production* p = &(*productions)[idx];
        char** first_symbols = malloc(p->rhs_count * sizeof(char*));
        int fs_count = 0;
        for (int j = 0; j < p->rhs_count; j++) {
            int sym_count;
            char** symbols = split_alternative(p->rhs[j], &sym_count);
            if (sym_count > 0) {
                char* first = symbols[0];
                if (!is_used(first_symbols, fs_count, first)) {
                    first_symbols[fs_count++] = strdup(first);
                }
            }
            for (int k = 0; k < sym_count; k++) free(symbols[k]);
            free(symbols);
        }
        char** new_rhs = malloc(p->rhs_count * sizeof(char*));
        int new_rhs_count = 0;
        for (int k = 0; k < fs_count; k++) {
            char* first_symbol = first_symbols[k];
            char** group_alts = malloc(p->rhs_count * sizeof(char*));
            int group_count = 0;
            for (int j = 0; j < p->rhs_count; j++) {
                int sym_count;
                char** symbols = split_alternative(p->rhs[j], &sym_count);
                if (sym_count > 0 && strcmp(symbols[0], first_symbol) == 0) {
                    group_alts[group_count++] = strdup(p->rhs[j]);
                }
                for (int m = 0; m < sym_count; m++) free(symbols[m]);
                free(symbols);
            }
            if (group_count > 1) {
                char* A_prime = generate_new_nt(A, used_nt, used_count);
                char** suffixes = malloc(group_count * sizeof(char*));
                for (int j = 0; j < group_count; j++) {
                    int sym_count;
                    char** symbols = split_alternative(group_alts[j], &sym_count);
                    suffixes[j] = join_symbols(symbols, 1, sym_count);
                    for (int m = 0; m < sym_count; m++) free(symbols[m]);
                    free(symbols);
                }
                *productions = realloc(*productions, (*prod_count + 1) * sizeof(Production));
                (*productions)[*prod_count].lhs = strdup(A_prime);
                (*productions)[*prod_count].rhs = suffixes;
                (*productions)[*prod_count].rhs_count = group_count;
                (*prod_count)++;
                enqueue(&q, A_prime);
                char* new_alt = malloc(strlen(first_symbol) + strlen(A_prime) + 2);
                sprintf(new_alt, "%s %s", first_symbol, A_prime);
                new_rhs[new_rhs_count++] = new_alt;
            } else if (group_count == 1) {
                new_rhs[new_rhs_count++] = strdup(group_alts[0]);
            }
            for (int j = 0; j < group_count; j++) free(group_alts[j]);
            free(group_alts);
        }
        for (int j = 0; j < p->rhs_count; j++) free(p->rhs[j]);
        free(p->rhs);
        p->rhs = new_rhs;
        p->rhs_count = new_rhs_count;
        for (int k = 0; k < fs_count; k++) free(first_symbols[k]);
        free(first_symbols);
        free(A);
    }
    free(q.nt_list);
}

// Remove left recursion
void remove_left_recursion(Production** productions, int* prod_count, char** used_nt, int* used_count) {
    for (int i = 0; i < *prod_count; i++) {
        Production* p = &(*productions)[i];
        char* A = p->lhs;
        char** alpha = malloc(p->rhs_count * sizeof(char*));
        int alpha_count = 0;
        char** beta = malloc(p->rhs_count * sizeof(char*));
        int beta_count = 0;
        for (int j = 0; j < p->rhs_count; j++) {
            int sym_count;
            char** symbols = split_alternative(p->rhs[j], &sym_count);
            if (sym_count > 0 && strcmp(symbols[0], A) == 0) {
                alpha[alpha_count++] = join_symbols(symbols, 1, sym_count);
            } else {
                beta[beta_count++] = strdup(p->rhs[j]);
            }
            for (int k = 0; k < sym_count; k++) free(symbols[k]);
            free(symbols);
        }
        if (alpha_count > 0) {
            char* A_prime = generate_new_nt(A, used_nt, used_count);
            char** new_A_rhs = malloc(beta_count * sizeof(char*));
            for (int j = 0; j < beta_count; j++) {
                new_A_rhs[j] = malloc(strlen(beta[j]) + strlen(A_prime) + 2);
                sprintf(new_A_rhs[j], "%s %s", beta[j], A_prime);
            }
            char** new_A_prime_rhs = malloc((alpha_count + 1) * sizeof(char*));
            for (int j = 0; j < alpha_count; j++) {
                new_A_prime_rhs[j] = malloc(strlen(alpha[j]) + strlen(A_prime) + 2);
                sprintf(new_A_prime_rhs[j], "%s %s", alpha[j], A_prime);
            }
            new_A_prime_rhs[alpha_count] = strdup("ε");
            for (int j = 0; j < p->rhs_count; j++) free(p->rhs[j]);
            free(p->rhs);
            p->rhs = new_A_rhs;
            p->rhs_count = beta_count;
            *productions = realloc(*productions, (*prod_count + 1) * sizeof(Production));
            (*productions)[*prod_count].lhs = strdup(A_prime);
            (*productions)[*prod_count].rhs = new_A_prime_rhs;
            (*productions)[*prod_count].rhs_count = alpha_count + 1;
            (*prod_count)++;
        }
        for (int j = 0; j < alpha_count; j++) free(alpha[j]);
        free(alpha);
        for (int j = 0; j < beta_count; j++) free(beta[j]);
        free(beta);
    }
}

void free_grammar(Production* productions, int prod_count, char** used_nt, int used_count) {
    for (int i = 0; i < prod_count; i++) {
        free(productions[i].lhs);
        for (int j = 0; j < productions[i].rhs_count; j++) {
            free(productions[i].rhs[j]);
        }
        free(productions[i].rhs);
    }
    free(productions);
    for (int i = 0; i < used_count; i++) free(used_nt[i]);
    free(used_nt);
}

// Main function
int main() {
    int prod_count, used_count;
    char** used_nt;

    // Step 1: Parse and show original grammar
    Production* productions = parse_grammar("input.txt", &prod_count, &used_nt, &used_count);
    printf("Original Grammar:\n");
    print_grammar(productions, prod_count);

    // Step 2: Apply left factoring and show result
    left_factoring(&productions, &prod_count, used_nt, &used_count);
    printf("\nAfter Left Factoring:\n");
    print_grammar(productions, prod_count);

    // Step 3: Apply left recursion removal on the factored grammar and show result
    remove_left_recursion(&productions, &prod_count, used_nt, &used_count);
    printf("\nAfter Left Recursion Removal:\n");
    print_grammar(productions, prod_count);

    // Clean up
    free_grammar(productions, prod_count, used_nt, used_count);
    return 0;
}