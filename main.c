#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_ROOMMATES    20
#define MAX_NAME_LEN     64
#define MAX_DESC_LEN    128
#define NAMES_FILE      "roommates.dat"
#define LEDGER_FILE     "ledger.txt"

typedef struct {
    char   name[MAX_NAME_LEN];
    double percentage;
    double amount_owed;
} Roommate;

/* ── Rounding ────────────────────────────────────────────────────────── */

/* Round x to 2 decimal places without requiring -lm */
static double round2(double x) {
    long cents = (long)(x * 100.0 + 0.5);
    return cents / 100.0;
}

/* ── String helpers ──────────────────────────────────────────────────── */

/* Strip leading and trailing whitespace in-place */
static void trim(char *s) {
    if (!s || *s == '\0') return;

    /* trailing */
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';

    /* leading */
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

/* Read one line from stdin into buf[size].
   Returns 1 on success (non-empty after trim), 0 on empty/EOF. */
static int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    buf[strcspn(buf, "\n")] = '\0';
    trim(buf);
    return buf[0] != '\0';
}

/* ── Input / Validation ──────────────────────────────────────────────── */

static double input_positive_double(const char *prompt) {
    char buf[128];
    char *end;
    double val;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!fgets(buf, sizeof buf, stdin)) {
            printf("\n  Input error. Try again.\n");
            continue;
        }
        buf[strcspn(buf, "\n")] = '\0';
        trim(buf);

        if (buf[0] == '\0') {
            printf("  Cannot be empty. Try again.\n");
            continue;
        }
        val = strtod(buf, &end);
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0') {
            printf("  Invalid input — please enter a number.\n");
            continue;
        }
        if (val <= 0.0) {
            printf("  Value must be greater than zero. Try again.\n");
            continue;
        }
        return val;
    }
}

static int input_positive_int(const char *prompt, int min_val, int max_val) {
    char buf[64];
    char *end;
    long val;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!fgets(buf, sizeof buf, stdin)) {
            printf("\n  Input error. Try again.\n");
            continue;
        }
        buf[strcspn(buf, "\n")] = '\0';
        trim(buf);

        if (buf[0] == '\0') {
            printf("  Cannot be empty. Try again.\n");
            continue;
        }
        val = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0') {
            printf("  Invalid input — please enter a whole number.\n");
            continue;
        }
        if (val < min_val || val > max_val) {
            printf("  Please enter a number between %d and %d.\n",
                   min_val, max_val);
            continue;
        }
        return (int)val;
    }
}

/* Prompt for a name; reject empty, all-whitespace, and duplicates. */
static void input_name(const char *prompt, char *out, int size,
                        Roommate *existing, int existing_count) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(out, size)) {
            printf("  Name cannot be empty. Try again.\n");
            continue;
        }

        /* Duplicate check */
        int dup = 0;
        for (int i = 0; i < existing_count; i++) {
            if (strcmp(existing[i].name, out) == 0) {
                dup = 1;
                break;
            }
        }
        if (dup) {
            printf("  That name is already used. Enter a different name.\n");
            continue;
        }
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
        trim(line);
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

static void append_ledger(Roommate *roommates, int count,
                           double total, const char *description) {
    FILE *f = fopen(LEDGER_FILE, "a");
    if (!f) {
        printf("  Warning: could not write to ledger (%s)\n", LEDGER_FILE);
        return;
    }

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char ts[32];
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", lt);

    fprintf(f, "========================================\n");
    fprintf(f, "Timestamp   : %s\n", ts);
    fprintf(f, "Description : %s\n", description);
    fprintf(f, "Total Bill  : $%.2f\n", total);
    fprintf(f, "----------------------------------------\n");
    fprintf(f, "%-22s  %7s  %12s\n", "Name", "Share %", "Amount Owed");
    fprintf(f, "%-22s  %7s  %12s\n",
            "----------------------", "-------", "------------");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%-22s  %6.2f%%  $%11.2f\n",
                roommates[i].name,
                roommates[i].percentage,
                roommates[i].amount_owed);
    }
    fprintf(f, "========================================\n\n");
    fclose(f);
}

/* ── Display Results ─────────────────────────────────────────────────── */

static void display_results(Roommate *roommates, int count,
                             double total, const char *description) {
    printf("\n");
    printf("========================================\n");
    printf("  Expense Split Results\n");
    printf("========================================\n");
    printf("  Description : %s\n", description);
    printf("  Total Bill  : $%.2f\n", total);
    printf("----------------------------------------\n");
    printf("  %-22s  %7s  %12s\n", "Name", "Share %", "Amount Owed");
    printf("  %-22s  %7s  %12s\n",
           "----------------------", "-------", "------------");
    for (int i = 0; i < count; i++) {
        printf("  %-22s  %6.2f%%  $%11.2f\n",
               roommates[i].name,
               roommates[i].percentage,
               roommates[i].amount_owed);
    }
    printf("========================================\n\n");
}

/* ── View Raw Ledger ─────────────────────────────────────────────────── */

static void view_ledger(void) {
    FILE *f = fopen(LEDGER_FILE, "r");
    if (!f) {
        printf("\n  No ledger entries found yet.\n\n");
        return;
    }
    printf("\n=== Full Expense Ledger ===\n\n");
    char line[256];
    while (fgets(line, sizeof line, f))
        printf("%s", line);
    fclose(f);
}

/* ── Per-Person Balance Summary ──────────────────────────────────────── */
/*
 * Enhancement: scan the entire ledger and accumulate each unique
 * person's total spending across all recorded transactions.
 */

#define MAX_SUMMARY_PEOPLE 100

static void view_balance_summary(void) {
    FILE *f = fopen(LEDGER_FILE, "r");
    if (!f) {
        printf("\n  No ledger entries found yet.\n\n");
        return;
    }

    char  summary_names[MAX_SUMMARY_PEOPLE][MAX_NAME_LEN];
    double summary_totals[MAX_SUMMARY_PEOPLE];
    int   summary_count = 0;
    double grand_total  = 0.0;

    /*
     * Ledger data lines have this fixed format (from fprintf):
     *   %-22s  %6.2f%%  $%11.2f
     * Column layout:
     *   [0..21]  name (22 chars, left-padded with spaces)
     *   [22..23] "  "
     *   [24..29] percentage digits (6 chars)
     *   [30]     '%'
     *   [31..32] "  "
     *   [33]     '$'
     *   [34..44] amount digits (11 chars)
     */
    char line[256];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\n")] = '\0';

        /* Quick structural check: must be at least 45 chars with '%' at [30]
           and '$' at [33]. */
        if ((int)strlen(line) < 45 || line[30] != '%' || line[33] != '$')
            continue;

        char pname[MAX_NAME_LEN];
        strncpy(pname, line, 22);
        pname[22] = '\0';
        trim(pname);

        /* Skip header/separator rows */
        if (pname[0] == '-' || strcmp(pname, "Name") == 0 || pname[0] == '\0')
            continue;

        char *end;
        double amt = strtod(line + 34, &end);
        if (end == line + 34) continue;   /* no number found */

        /* Find or add person */
        int found = -1;
        for (int i = 0; i < summary_count; i++) {
            if (strcmp(summary_names[i], pname) == 0) {
                found = i;
                break;
            }
        }
        if (found == -1 && summary_count < MAX_SUMMARY_PEOPLE) {
            strncpy(summary_names[summary_count], pname, MAX_NAME_LEN - 1);
            summary_names[summary_count][MAX_NAME_LEN - 1] = '\0';
            summary_totals[summary_count] = 0.0;
            found = summary_count++;
        }
        if (found != -1) {
            summary_totals[found] += amt;
            grand_total           += amt;
        }
    }
    fclose(f);

    if (summary_count == 0) {
        printf("\n  No balance data could be read from the ledger.\n\n");
        return;
    }

    printf("\n========================================\n");
    printf("  Per-Person Balance Summary\n");
    printf("========================================\n");
    printf("  %-22s  %12s  %7s\n", "Name", "Total Paid", "% of All");
    printf("  %-22s  %12s  %7s\n",
           "----------------------", "------------", "-------");
    for (int i = 0; i < summary_count; i++) {
        double share = (grand_total > 0.0)
                       ? summary_totals[i] / grand_total * 100.0
                       : 0.0;
        printf("  %-22s  $%11.2f  %6.2f%%\n",
               summary_names[i], summary_totals[i], share);
    }
    printf("  %-22s  %12s  %7s\n",
           "----------------------", "------------", "-------");
    printf("  %-22s  $%11.2f  %6.2f%%\n",
           "Grand Total", grand_total, 100.0);
    printf("========================================\n\n");
}

/* ── Core Split Logic ────────────────────────────────────────────────── */

static void do_expense_split(void) {
    Roommate roommates[MAX_ROOMMATES];
    char     saved_names[MAX_ROOMMATES][MAX_NAME_LEN];
    char     description[MAX_DESC_LEN];

    int saved_count = load_saved_names(saved_names, MAX_ROOMMATES);

    printf("\n--- New Expense Split ---\n\n");

    /* Description */
    for (;;) {
        printf("Enter a description (e.g., Rent, Groceries): ");
        fflush(stdout);
        if (read_line(description, sizeof description)) break;
        printf("  Description cannot be empty. Try again.\n");
    }

    /* Total bill */
    double total = input_positive_double("Enter the total bill amount ($): ");

    /* Number of people */
    int count = input_positive_int(
        "Enter the number of people splitting this bill: ", 1, MAX_ROOMMATES);

    /* Names — show saved list as reference, offer reuse option */
    printf("\n");
    if (saved_count > 0) {
        printf("  Saved roommates: ");
        for (int s = 0; s < saved_count; s++) {
            printf("%s", saved_names[s]);
            if (s < saved_count - 1) printf(", ");
        }
        printf("\n");

        if (count <= saved_count) {
            printf("  Use saved names for all %d people? (y/n): ", count);
            fflush(stdout);
            char yn[8];
            if (fgets(yn, sizeof yn, stdin)) {
                yn[strcspn(yn, "\n")] = '\0';
                if (tolower((unsigned char)yn[0]) == 'y') {
                    for (int i = 0; i < count; i++) {
                        strncpy(roommates[i].name, saved_names[i],
                                MAX_NAME_LEN - 1);
                        roommates[i].name[MAX_NAME_LEN - 1] = '\0';
                        printf("    Loaded: %s\n", roommates[i].name);
                    }
                    goto names_done;
                }
            }
        }
    }

    for (int i = 0; i < count; i++) {
        char prompt[80];
        snprintf(prompt, sizeof prompt, "  Person %d — enter name: ", i + 1);
        input_name(prompt, roommates[i].name, MAX_NAME_LEN, roommates, i);
    }

names_done:

    /* Equal or weighted split */
    printf("\nSplit equally among all %d people? (y/n): ", count);
    fflush(stdout);
    char choice[8] = "y";
    if (fgets(choice, sizeof choice, stdin))
        choice[strcspn(choice, "\n")] = '\0';

    if (tolower((unsigned char)choice[0]) == 'y') {
        double equal_pct = 100.0 / count;
        for (int i = 0; i < count; i++)
            roommates[i].percentage = equal_pct;
    } else {
        printf("\nEnter each person's percentage share (must total 100%%):\n");
        double total_pct = 0.0;

        for (int i = 0; i < count; i++) {
            char prompt[128];
            snprintf(prompt, sizeof prompt,
                     "  Percentage for %-20s: ", roommates[i].name);
            double pct;
            for (;;) {
                pct = input_positive_double(prompt);
                if (pct > 100.0) {
                    printf("  Percentage cannot exceed 100. Try again.\n");
                    continue;
                }
                /* Ensure remaining budget isn't already spent */
                if (i < count - 1 && total_pct + pct >= 100.0) {
                    printf("  Running total would reach %.2f%% with %d "
                           "person(s) still remaining. Try again.\n",
                           total_pct + pct, count - 1 - i);
                    continue;
                }
                break;
            }
            roommates[i].percentage = pct;
            total_pct += pct;
        }

        if (total_pct < 99.995 || total_pct > 100.005) {
            printf("\n  Error: percentages sum to %.4f%% instead of 100%%.\n",
                   total_pct);
            printf("  Split cancelled. Returning to menu.\n\n");
            return;
        }
    }

    /* Calculate amounts — last person absorbs any rounding remainder */
    double running = 0.0;
    for (int i = 0; i < count - 1; i++) {
        roommates[i].amount_owed = round2(roommates[i].percentage / 100.0 * total);
        running += roommates[i].amount_owed;
    }
    roommates[count - 1].amount_owed = round2(total - running);

    /* Output and persist */
    display_results(roommates, count, total, description);
    append_ledger(roommates, count, total, description);
    save_names(roommates, count);
    printf("  Saved to ledger. Roommate names updated.\n\n");
}

/* ── Main Menu ───────────────────────────────────────────────────────── */

int main(void) {
    printf("============================================\n");
    printf("        Expense Splitter  —  CEP Lab\n");
    printf("============================================\n\n");

    for (;;) {
        printf("  1. Split a new expense\n");
        printf("  2. View full expense ledger\n");
        printf("  3. View per-person balance summary\n");
        printf("  4. Exit\n");
        printf("\nChoose an option (1-4): ");
        fflush(stdout);

        char line[16];
        if (!fgets(line, sizeof line, stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        trim(line);

        if (strcmp(line, "1") == 0) {
            do_expense_split();
        } else if (strcmp(line, "2") == 0) {
            view_ledger();
        } else if (strcmp(line, "3") == 0) {
            view_balance_summary();
        } else if (strcmp(line, "4") == 0) {
            printf("\n  Goodbye!\n\n");
            break;
        } else {
            printf("  Invalid choice. Please enter 1, 2, 3, or 4.\n\n");
        }
    }

    return 0;
}
