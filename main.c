#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_ROOMMATES 20
#define MAX_NAME_LEN 64
#define NAMES_FILE "roommates.dat"
#define LEDGER_FILE "ledger.txt"

typedef struct {
    char name[MAX_NAME_LEN];
    double percentage;
    double amount_owed;
} Roommate;

/* ── Utility ─────────────────────────────────────────────────────────── */

/* Read a non-empty line; returns 1 on success, 0 on EOF */
static int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin))
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[--len] = '\0';
    if (len == 0) {
        /* empty line — keep asking */
        return 0;
    }
    return 1;
}

/* ── Input / Validation ──────────────────────────────────────────────── */

static double input_positive_double(const char *prompt) {
    char buf[128];
    double val;
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) {
            printf("  Input error. Try again.\n");
            continue;
        }
        /* strip newline */
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0') {
            printf("  Cannot be empty. Try again.\n");
            continue;
        }
        val = strtod(buf, &end);
        /* end must point past whitespace only */
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0') {
            printf("  Invalid input. Please enter a number.\n");
            continue;
        }
        if (val <= 0.0) {
            printf("  Value must be greater than zero. Try again.\n");
            continue;
        }
        return val;
    }
}

static int input_positive_int(const char *prompt, int max) {
    char buf[64];
    long val;
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) {
            printf("  Input error. Try again.\n");
            continue;
        }
        buf[strcspn(buf, "\n")] = '\0';
        if (buf[0] == '\0') {
            printf("  Cannot be empty. Try again.\n");
            continue;
        }
        val = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0') {
            printf("  Invalid input. Please enter a whole number.\n");
            continue;
        }
        if (val < 1) {
            printf("  Must be at least 1. Try again.\n");
            continue;
        }
        if (max > 0 && val > max) {
            printf("  Maximum allowed is %d. Try again.\n", max);
            continue;
        }
        return (int)val;
    }
}

static void input_name(const char *prompt, char *out, int size) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(out, size)) {
            printf("  Name cannot be empty. Try again.\n");
            continue;
        }
        /* trim leading spaces */
        char *p = out;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') {
            printf("  Name cannot be blank. Try again.\n");
            continue;
        }
        if (p != out) memmove(out, p, strlen(p) + 1);
        break;
    }
}

/* ── Persistent Roommate Names ───────────────────────────────────────── */

static int load_saved_names(char names[][MAX_NAME_LEN], int max_count) {
    FILE *f = fopen(NAMES_FILE, "r");
    if (!f) return 0;

    int count = 0;
    char line[MAX_NAME_LEN + 4];
    while (count < max_count && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] != '\0') {
            strncpy(names[count], line, MAX_NAME_LEN - 1);
            names[count][MAX_NAME_LEN - 1] = '\0';
            count++;
        }
    }
    fclose(f);
    return count;
}

static void save_names(Roommate *roommates, int count) {
    FILE *f = fopen(NAMES_FILE, "w");
    if (!f) {
        printf("  Warning: could not save roommate names to %s\n", NAMES_FILE);
        return;
    }
    for (int i = 0; i < count; i++)
        fprintf(f, "%s\n", roommates[i].name);
    fclose(f);
}

/* ── Ledger ──────────────────────────────────────────────────────────── */

static void append_ledger(Roommate *roommates, int count, double total,
                           const char *description) {
    FILE *f = fopen(LEDGER_FILE, "a");
    if (!f) {
        printf("  Warning: could not write to ledger file %s\n", LEDGER_FILE);
        return;
    }

    time_t now = time(NULL);
    char ts[64];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(f, "========================================\n");
    fprintf(f, "Timestamp   : %s\n", ts);
    fprintf(f, "Description : %s\n", description);
    fprintf(f, "Total Bill  : $%.2f\n", total);
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "%-20s  %8s  %12s\n", "Name", "Share %", "Amount Owed");
    fprintf(f, "%-20s  %8s  %12s\n", "--------------------", "--------", "------------");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%-20s  %7.2f%%  $%11.2f\n",
                roommates[i].name,
                roommates[i].percentage,
                roommates[i].amount_owed);
    }
    fprintf(f, "========================================\n\n");
    fclose(f);
}

/* ── Display ─────────────────────────────────────────────────────────── */

static void display_results(Roommate *roommates, int count, double total,
                             const char *description) {
    printf("\n");
    printf("========================================\n");
    printf("  Expense Split Results\n");
    printf("========================================\n");
    printf("  Description : %s\n", description);
    printf("  Total Bill  : $%.2f\n", total);
    printf("----------------------------------------\n");
    printf("  %-20s  %8s  %12s\n", "Name", "Share %", "Amount Owed");
    printf("  %-20s  %8s  %12s\n", "--------------------", "--------", "------------");
    for (int i = 0; i < count; i++) {
        printf("  %-20s  %7.2f%%  $%11.2f\n",
               roommates[i].name,
               roommates[i].percentage,
               roommates[i].amount_owed);
    }
    printf("========================================\n\n");
}

static void view_ledger(void) {
    FILE *f = fopen(LEDGER_FILE, "r");
    if (!f) {
        printf("\n  No ledger entries found yet.\n\n");
        return;
    }
    printf("\n--- Expense Ledger ---\n\n");
    char line[256];
    while (fgets(line, sizeof line, f))
        printf("%s", line);
    fclose(f);
}

/* ── Core Split Logic ────────────────────────────────────────────────── */

static void do_expense_split(void) {
    Roommate roommates[MAX_ROOMMATES];
    char saved_names[MAX_ROOMMATES][MAX_NAME_LEN];
    char description[128];

    /* Load previously saved names */
    int saved_count = load_saved_names(saved_names, MAX_ROOMMATES);

    printf("\n--- New Expense Split ---\n\n");

    /* Description */
    printf("Enter a description (e.g., Rent, Groceries): ");
    fflush(stdout);
    for (;;) {
        if (read_line(description, sizeof description) && description[0] != '\0')
            break;
        printf("  Description cannot be empty. Try again: ");
        fflush(stdout);
    }

    /* Total bill */
    double total = input_positive_double("Enter the total bill amount ($): ");

    /* Number of roommates */
    int count = input_positive_int(
        "Enter the number of people splitting this bill: ", MAX_ROOMMATES);

    /* Collect names — offer saved names as shortcuts */
    printf("\n");
    for (int i = 0; i < count; i++) {
        printf("  Person %d", i + 1);

        /* Show saved names if any remain */
        if (saved_count > 0) {
            printf(" (saved names: ");
            for (int s = 0; s < saved_count; s++) {
                printf("%s", saved_names[s]);
                if (s < saved_count - 1) printf(", ");
            }
            printf(")");
        }
        printf("\n");

        char prompt[64];
        snprintf(prompt, sizeof prompt, "    Enter name: ");
        input_name(prompt, roommates[i].name, MAX_NAME_LEN);
    }

    /* Percentages — equal split or weighted */
    printf("\nWould you like to split equally? (y/n): ");
    fflush(stdout);
    char choice[8];
    fgets(choice, sizeof choice, stdin);
    choice[strcspn(choice, "\n")] = '\0';

    if (tolower((unsigned char)choice[0]) == 'y') {
        double equal_pct = 100.0 / count;
        for (int i = 0; i < count; i++)
            roommates[i].percentage = equal_pct;
    } else {
        double total_pct = 0.0;
        printf("\nEnter each person's percentage share (must total 100%%):\n");
        for (int i = 0; i < count; i++) {
            char prompt[128];
            snprintf(prompt, sizeof prompt,
                     "  Percentage for %s: ", roommates[i].name);

            for (;;) {
                double pct = input_positive_double(prompt);
                if (pct > 100.0) {
                    printf("  Percentage cannot exceed 100. Try again.\n");
                    continue;
                }
                roommates[i].percentage = pct;
                total_pct += pct;
                break;
            }
        }

        /* Validate total */
        if (total_pct < 99.99 || total_pct > 100.01) {
            printf("\n  Error: percentages sum to %.2f%% instead of 100%%.\n",
                   total_pct);
            printf("  Split cancelled. Returning to menu.\n\n");
            return;
        }
        /* Normalise minor floating-point drift */
        double scale = 100.0 / total_pct;
        for (int i = 0; i < count; i++)
            roommates[i].percentage *= scale;
    }

    /* Calculate amounts */
    double running_total = 0.0;
    for (int i = 0; i < count - 1; i++) {
        roommates[i].amount_owed =
            (int)((roommates[i].percentage / 100.0 * total) * 100 + 0.5) / 100.0;
        running_total += roommates[i].amount_owed;
    }
    /* Last person gets the remainder to avoid rounding gaps */
    roommates[count - 1].amount_owed =
        (int)((total - running_total) * 100 + 0.5) / 100.0;

    /* Display and save */
    display_results(roommates, count, total, description);
    append_ledger(roommates, count, total, description);
    save_names(roommates, count);
    printf("  Results saved to ledger. Roommate names updated.\n\n");
}

/* ── Main Menu ───────────────────────────────────────────────────────── */

int main(void) {
    printf("============================================\n");
    printf("       Expense Splitter — CEP Lab\n");
    printf("============================================\n");

    for (;;) {
        printf("  1. Split a new expense\n");
        printf("  2. View expense ledger\n");
        printf("  3. Exit\n");
        printf("Choose an option: ");
        fflush(stdout);

        char line[16];
        if (!fgets(line, sizeof line, stdin)) break;
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "1") == 0) {
            do_expense_split();
        } else if (strcmp(line, "2") == 0) {
            view_ledger();
        } else if (strcmp(line, "3") == 0) {
            printf("\n  Goodbye!\n\n");
            break;
        } else {
            printf("  Invalid option. Please enter 1, 2, or 3.\n\n");
        }
    }

    return 0;
}
