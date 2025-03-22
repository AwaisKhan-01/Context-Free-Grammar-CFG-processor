#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Structure for a production
typedef struct {
    char* lhs;
    char** rhs;
    int rhs_count;
} Production;

// Helper Functions

char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

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

char** split_alternative(char* alt, int* count) {
    return split_string(alt, " ", count);
}

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

int is_used(char** used_nt, int used_count, char* name) {
    for (int i = 0; i < used_count; i++) {
        if (strcmp(used_nt[i], name) == 0) return 1;
    }
    return 0;
}

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

void print_grammar(FILE* fp, Production* productions, int prod_count, const char* stage) {
    fprintf(fp, "%s:\n", stage);
    for (int i = 0; i < prod_count; i++) {
        fprintf(fp, "%s -> ", productions[i].lhs);
        for (int j = 0; j < productions[i].rhs_count; j++) {
            fprintf(fp, "%s", productions[i].rhs[j]);
            if (j < productions[i].rhs_count - 1) fprintf(fp, " | ");
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

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

int get_nt_index(char* nt, char** non_terminals, int nt_count) {
    for (int i = 0; i < nt_count; i++) {
        if (strcmp(non_terminals[i], nt) == 0) return i;
    }
    return -1;
}

int is_nonterminal(char* symbol, char** non_terminals, int nt_count) {
    return get_nt_index(symbol, non_terminals, nt_count) != -1;
}

int is_terminal(char* symbol, char** non_terminals, int nt_count) {
    return strcmp(symbol, "ε") != 0 && !is_nonterminal(symbol, non_terminals, nt_count);
}

void add_to_set(char*** set, int* size, char* element, int capacity) {
    for (int i = 0; i < *size; i++) {
        if (strcmp((*set)[i], element) == 0) return;
    }
    if (*size < capacity) {
        (*set)[*size] = strdup(element);
        (*size)++;
    } else {
        printf("Warning: Set capacity exceeded for element %s\n", element);
    }
}

int* compute_nullable(Production* productions, int prod_count, char** non_terminals, int nt_count) {
    int* nullable = calloc(nt_count, sizeof(int));
    int changes;
    do {
        changes = 0;
        for (int i = 0; i < prod_count; i++) {
            Production* p = &productions[i];
            int A_idx = get_nt_index(p->lhs, non_terminals, nt_count);
            for (int j = 0; j < p->rhs_count; j++) {
                char* rhs = p->rhs[j];
                if (strcmp(rhs, "ε") == 0) {
                    if (!nullable[A_idx]) {
                        nullable[A_idx] = 1;
                        changes = 1;
                    }
                } else {
                    int sym_count;
                    char** symbols = split_alternative(rhs, &sym_count);
                    int all_nullable = 1;
                    for (int k = 0; k < sym_count; k++) {
                        char* symbol = symbols[k];
                        if (is_terminal(symbol, non_terminals, nt_count)) {
                            all_nullable = 0;
                            break;
                        } else {
                            int sym_idx = get_nt_index(symbol, non_terminals, nt_count);
                            if (!nullable[sym_idx]) {
                                all_nullable = 0;
                                break;
                            }
                        }
                    }
                    if (all_nullable && !nullable[A_idx]) {
                        nullable[A_idx] = 1;
                        changes = 1;
                    }
                    for (int k = 0; k < sym_count; k++) free(symbols[k]);
                    free(symbols);
                }
            }
        }
    } while (changes);
    return nullable;
}

void compute_first_sets(FILE* fp, Production* productions, int prod_count, char** non_terminals, int nt_count, int* nullable, char*** first_sets, int* first_sizes, int first_capacity) {
    int changes;
    do {
        changes = 0;
        for (int i = 0; i < prod_count; i++) {
            Production* p = &productions[i];
            int A_idx = get_nt_index(p->lhs, non_terminals, nt_count);
            for (int j = 0; j < p->rhs_count; j++) {
                char* rhs = p->rhs[j];
                if (strcmp(rhs, "ε") == 0) continue;
                int sym_count;
                char** symbols = split_alternative(rhs, &sym_count);
                for (int k = 0; k < sym_count; k++) {
                    char* symbol = symbols[k];
                    if (is_terminal(symbol, non_terminals, nt_count)) {
                        int initial_size = first_sizes[A_idx];
                        add_to_set(&first_sets[A_idx], &first_sizes[A_idx], symbol, first_capacity);
                        if (first_sizes[A_idx] > initial_size) changes = 1;
                        break;
                    } else {
                        int sym_idx = get_nt_index(symbol, non_terminals, nt_count);
                        for (int m = 0; m < first_sizes[sym_idx]; m++) {
                            int initial_size = first_sizes[A_idx];
                            add_to_set(&first_sets[A_idx], &first_sizes[A_idx], first_sets[sym_idx][m], first_capacity);
                            if (first_sizes[A_idx] > initial_size) changes = 1;
                        }
                        if (!nullable[sym_idx]) break;
                    }
                }
                for (int k = 0; k < sym_count; k++) free(symbols[k]);
                free(symbols);
            }
        }
    } while (changes);

    fprintf(fp, "First Sets:\n");
    for (int i = 0; i < nt_count; i++) {
        fprintf(fp, "First(%s) = { ", non_terminals[i]);
        for (int j = 0; j < first_sizes[i]; j++) {
            fprintf(fp, "%s", first_sets[i][j]);
            if (j < first_sizes[i] - 1) fprintf(fp, ", ");
        }
        fprintf(fp, " }\n");
    }
    fprintf(fp, "\n");
}

void compute_follow_sets(FILE* fp, Production* productions, int prod_count, char** non_terminals, int nt_count, int* nullable, char*** first_sets, int* first_sizes, char*** follow_sets, int* follow_sizes, int follow_capacity) {
    int start_idx = 0;
    add_to_set(&follow_sets[start_idx], &follow_sizes[start_idx], "$", follow_capacity);

    int changes;
    do {
        changes = 0;
        for (int i = 0; i < prod_count; i++) {
            Production* p = &productions[i];
            int B_idx = get_nt_index(p->lhs, non_terminals, nt_count);
            for (int j = 0; j < p->rhs_count; j++) {
                char* rhs = p->rhs[j];
                if (strcmp(rhs, "ε") == 0) continue;
                int sym_count;
                char** symbols = split_alternative(rhs, &sym_count);
                for (int k = 0; k < sym_count; k++) {
                    if (is_nonterminal(symbols[k], non_terminals, nt_count)) {
                        int A_idx = get_nt_index(symbols[k], non_terminals, nt_count);
                        int beta_nullable = 1;
                        for (int m = k + 1; m < sym_count; m++) {
                            char* beta_symbol = symbols[m];
                            if (is_terminal(beta_symbol, non_terminals, nt_count)) {
                                int initial_size = follow_sizes[A_idx];
                                add_to_set(&follow_sets[A_idx], &follow_sizes[A_idx], beta_symbol, follow_capacity);
                                if (follow_sizes[A_idx] > initial_size) changes = 1;
                                beta_nullable = 0;
                                break;
                            } else {
                                int beta_idx = get_nt_index(beta_symbol, non_terminals, nt_count);
                                for (int n = 0; n < first_sizes[beta_idx]; n++) {
                                    int initial_size = follow_sizes[A_idx];
                                    add_to_set(&follow_sets[A_idx], &follow_sizes[A_idx], first_sets[beta_idx][n], follow_capacity);
                                    if (follow_sizes[A_idx] > initial_size) changes = 1;
                                }
                                if (!nullable[beta_idx]) {
                                    beta_nullable = 0;
                                    break;
                                }
                            }
                        }
                        if (beta_nullable) {
                            for (int n = 0; n < follow_sizes[B_idx]; n++) {
                                int initial_size = follow_sizes[A_idx];
                                add_to_set(&follow_sets[A_idx], &follow_sizes[A_idx], follow_sets[B_idx][n], follow_capacity);
                                if (follow_sizes[A_idx] > initial_size) changes = 1;
                            }
                        }
                    }
                }
                for (int k = 0; k < sym_count; k++) free(symbols[k]);
                free(symbols);
            }
        }
    } while (changes);

    fprintf(fp, "Follow Sets:\n");
    for (int i = 0; i < nt_count; i++) {
        fprintf(fp, "Follow(%s) = { ", non_terminals[i]);
        for (int j = 0; j < follow_sizes[i]; j++) {
            fprintf(fp, "%s", follow_sets[i][j]);
            if (j < follow_sizes[i] - 1) fprintf(fp, ", ");
        }
        fprintf(fp, " }\n");
    }
    fprintf(fp, "\n");
}

char** collect_terminals(Production* productions, int prod_count, char** non_terminals, int nt_count, int* term_count) {
    char** terminals = malloc(100 * sizeof(char*));
    int capacity = 100;
    *term_count = 0;
    for (int i = 0; i < prod_count; i++) {
        for (int j = 0; j < productions[i].rhs_count; j++) {
            int sym_count;
            char** symbols = split_alternative(productions[i].rhs[j], &sym_count);
            for (int k = 0; k < sym_count; k++) {
                char* symbol = symbols[k];
                if (is_terminal(symbol, non_terminals, nt_count)) {
                    if (!is_used(terminals, *term_count, symbol)) {
                        if (*term_count >= capacity) {
                            capacity *= 2;
                            terminals = realloc(terminals, capacity * sizeof(char*));
                        }
                        terminals[*term_count] = strdup(symbol);
                        (*term_count)++;
                    }
                }
            }
            for (int k = 0; k < sym_count; k++) free(symbols[k]);
            free(symbols);
        }
    }
    if (!is_used(terminals, *term_count, "$")) {
        if (*term_count >= capacity) {
            terminals = realloc(terminals, (capacity + 1) * sizeof(char*));
        }
        terminals[*term_count] = strdup("$");
        (*term_count)++;
    }
    return terminals;
}

void compute_first_alpha(char* alpha, char** non_terminals, int nt_count, int* nullable, char*** first_sets, int* first_sizes, char** first_alpha, int* first_alpha_size, int capacity) {
    *first_alpha_size = 0;
    if (strcmp(alpha, "ε") == 0) return;
    int sym_count;
    char** symbols = split_alternative(alpha, &sym_count);
    for (int k = 0; k < sym_count; k++) {
        char* symbol = symbols[k];
        if (is_terminal(symbol, non_terminals, nt_count)) {
            add_to_set(&first_alpha, first_alpha_size, symbol, capacity);
            break;
        } else {
            int sym_idx = get_nt_index(symbol, non_terminals, nt_count);
            for (int m = 0; m < first_sizes[sym_idx]; m++) {
                add_to_set(&first_alpha, first_alpha_size, first_sets[sym_idx][m], capacity);
            }
            if (!nullable[sym_idx]) break;
        }
    }
    for (int k = 0; k < sym_count; k++) free(symbols[k]);
    free(symbols);
}

int is_alpha_nullable(char* alpha, char** non_terminals, int nt_count, int* nullable) {
    if (strcmp(alpha, "ε") == 0) return 1;
    int sym_count;
    char** symbols = split_alternative(alpha, &sym_count);
    int all_nullable = 1;
    for (int k = 0; k < sym_count; k++) {
        if (is_terminal(symbols[k], non_terminals, nt_count)) {
            all_nullable = 0;
            break;
        } else {
            int sym_idx = get_nt_index(symbols[k], non_terminals, nt_count);
            if (!nullable[sym_idx]) {
                all_nullable = 0;
                break;
            }
        }
    }
    for (int k = 0; k < sym_count; k++) free(symbols[k]);
    free(symbols);
    return all_nullable;
}

int** construct_ll1_table(Production* productions, int prod_count, char** non_terminals, int nt_count, char** terminals, int term_count, int* nullable, char*** first_sets, int* first_sizes, char*** follow_sets, int* follow_sizes) {
    int** table = malloc(nt_count * sizeof(int*));
    for (int i = 0; i < nt_count; i++) {
        table[i] = malloc(term_count * sizeof(int));
        for (int j = 0; j < term_count; j++) {
            table[i][j] = -1;
        }
    }

    int conflict = 0;
    for (int i = 0; i < prod_count; i++) {
        Production* p = &productions[i];
        int A_idx = get_nt_index(p->lhs, non_terminals, nt_count);
        for (int j = 0; j < p->rhs_count; j++) {
            char* alpha = p->rhs[j];
            char* first_alpha[10];
            int first_alpha_size = 0;
            compute_first_alpha(alpha, non_terminals, nt_count, nullable, first_sets, first_sizes, first_alpha, &first_alpha_size, 10);

            for (int k = 0; k < first_alpha_size; k++) {
                int t_idx = get_nt_index(first_alpha[k], terminals, term_count);
                if (t_idx != -1) {
                    if (table[A_idx][t_idx] != -1) {
                        printf("Conflict at [%s, %s]: Multiple productions (%d and %d)\n", non_terminals[A_idx], terminals[t_idx], table[A_idx][t_idx], i * 100 + j);
                        conflict = 1;
                    } else {
                        table[A_idx][t_idx] = i * 100 + j;
                    }
                }
            }
            for (int k = 0; k < first_alpha_size; k++) free(first_alpha[k]);

            if (is_alpha_nullable(alpha, non_terminals, nt_count, nullable)) {
                for (int k = 0; k < follow_sizes[A_idx]; k++) {
                    int t_idx = get_nt_index(follow_sets[A_idx][k], terminals, term_count);
                    if (t_idx != -1) {
                        if (table[A_idx][t_idx] != -1) {
                            printf("Conflict at [%s, %s]: Multiple productions (%d and %d)\n", non_terminals[A_idx], terminals[t_idx], table[A_idx][t_idx], i * 100 + j);
                            conflict = 1;
                        } else {
                            table[A_idx][t_idx] = i * 100 + j;
                        }
                    }
                }
            }
        }
    }
    if (conflict) {
        printf("Warning: Grammar is not LL(1) due to conflicts.\n");
    }
    return table;
}

void print_ll1_table(FILE* fp, int** table, Production* productions, int prod_count, char** non_terminals, int nt_count, char** terminals, int term_count) {
    fprintf(fp, "LL(1) Parsing Table:\n");
    fprintf(fp, "NT\\T ");
    for (int j = 0; j < term_count; j++) {
        fprintf(fp, "%-8s", terminals[j]);
    }
    fprintf(fp, "\n");
    for (int i = 0; i < nt_count; i++) {
        fprintf(fp, "%-4s ", non_terminals[i]);
        for (int j = 0; j < term_count; j++) {
            int entry = table[i][j];
            if (entry == -1) {
                fprintf(fp, "%-8s", "");
            } else {
                int prod_idx = entry / 100;
                int alt_idx = entry % 100;
                fprintf(fp, "%s->%-4s", productions[prod_idx].lhs, productions[prod_idx].rhs[alt_idx]);
            }
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
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

int main() {
    int prod_count, used_count;
    char** used_nt;

    // Open output log file
    FILE* fp = fopen("output_log.txt", "w");
    if (!fp) {
        printf("Error opening output_log.txt\n");
        exit(1);
    }

    // Step 1: Parse and log original grammar
    Production* productions = parse_grammar("input.txt", &prod_count, &used_nt, &used_count);
    print_grammar(fp, productions, prod_count, "Original Grammar");

    // Step 2: Apply left factoring and log result
    left_factoring(&productions, &prod_count, used_nt, &used_count);
    print_grammar(fp, productions, prod_count, "After Left Factoring");

    // Step 3: Apply left recursion removal and log result
    remove_left_recursion(&productions, &prod_count, used_nt, &used_count);
    print_grammar(fp, productions, prod_count, "After Left Recursion Removal");

    // Step 4: Compute and log First Sets
    int nt_count = used_count;
    char** non_terminals = used_nt;
    int* nullable = compute_nullable(productions, prod_count, non_terminals, nt_count);
    char*** first_sets = malloc(nt_count * sizeof(char**));
    int* first_sizes = calloc(nt_count, sizeof(int));
    int first_capacity = 10;
    for (int i = 0; i < nt_count; i++) {
        first_sets[i] = malloc(first_capacity * sizeof(char*));
    }
    compute_first_sets(fp, productions, prod_count, non_terminals, nt_count, nullable, first_sets, first_sizes, first_capacity);

    // Step 5: Compute and log Follow Sets
    char*** follow_sets = malloc(nt_count * sizeof(char**));
    int* follow_sizes = calloc(nt_count, sizeof(int));
    int follow_capacity = 10;
    for (int i = 0; i < nt_count; i++) {
        follow_sets[i] = malloc(follow_capacity * sizeof(char*));
    }
    compute_follow_sets(fp, productions, prod_count, non_terminals, nt_count, nullable, first_sets, first_sizes, follow_sets, follow_sizes, follow_capacity);

    // Step 6: Construct and log LL(1) Parsing Table
    int term_count;
    char** terminals = collect_terminals(productions, prod_count, non_terminals, nt_count, &term_count);
    int** ll1_table = construct_ll1_table(productions, prod_count, non_terminals, nt_count, terminals, term_count, nullable, first_sets, first_sizes, follow_sets, follow_sizes);
    print_ll1_table(fp, ll1_table, productions, prod_count, non_terminals, nt_count, terminals, term_count);

    // Clean up
    for (int i = 0; i < nt_count; i++) {
        for (int j = 0; j < first_sizes[i]; j++) free(first_sets[i][j]);
        free(first_sets[i]);
        for (int j = 0; j < follow_sizes[i]; j++) free(follow_sets[i][j]);
        free(follow_sets[i]);
        free(ll1_table[i]);
    }
    free(first_sets);
    free(first_sizes);
    free(follow_sets);
    free(follow_sizes);
    free(nullable);
    for (int i = 0; i < term_count; i++) free(terminals[i]);
    free(terminals);
    free(ll1_table);
    free_grammar(productions, prod_count, used_nt, used_count);

    fclose(fp);
    printf("Processing complete. Output written to output_log.txt\n");
    return 0;
}