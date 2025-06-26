#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h> 

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <conio.h>
    #define SLEEP(x) Sleep((x) * 1000)
#else
    #include <termios.h>
    #include <unistd.h>
    #define SLEEP(x) sleep(x)
#endif

#define ADMIN_REGISTRATION_KEY "TradeX123"
#define USER_DATA_DIR "user_data/"
#define STOCK_FILE "stocks.txt"
#define TRANSACTION_FILE "transactions.txt"
#define PORTFOLIO_FILE "portfolio.txt"
#define REGISTRATION_FILE "registration.txt"
#define USER_STOCK_FILE "user_stocks.txt"
#define HASH_TABLE_SIZE 100
#define MAX_LOGIN_ATTEMPTS 3
#define MAX_TRANSACTION_TYPE_LENGTH 10
#define MAX_STOCK_SYMBOL_LENGTH 10
#define MAX_STATUS_LENGTH 20
#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "Admin@1234"
#define ADMIN_SECRET_KEY "deepak"
#define MAX_ATTEMPTS 3
#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define WHITE   "\033[37m"
#define BG_BLACK "\033[40m"
#define GREEN   "\033[0;32m"
#define RESET   "\033[0m"

// ================== Data Structures ==================

typedef struct Stock {
    char symbol[10];
    char name[100];
    double price;
    float priceChange;
    float high52Week;
    float low52Week;
    char marketCap[20];
    float peRatio;
    float dividendYield;
    float eps;
    char volume[20];
    char avgVolume[20];
    char overview[256];
    char analystRating[20];
    struct Stock* left;
    struct Stock* right;
    int height;
} Stock;

typedef struct MoneyTransaction {
    char type[10]; 
    double amount;
    double balance_after;
    char description[100];
    time_t timestamp;
    struct MoneyTransaction* next;
} MoneyTransaction;

typedef struct StockTransaction {
    char type[MAX_TRANSACTION_TYPE_LENGTH]; 
    char stock_symbol[MAX_STOCK_SYMBOL_LENGTH];
    int quantity;
    double price;
    double total_amount;
    char status[MAX_STATUS_LENGTH]; 
    time_t timestamp;
    struct StockTransaction* next;
} StockTransaction;

typedef struct User {
    char username[50];
    char email[100];
    char password[100];
    double balance;  
    int failed_attempts;
    time_t last_login;
    char role[10]; 
    
    MoneyTransaction* transactions;
    StockTransaction* stockTransactions;  
    struct User* next;
} User;

typedef struct Transaction {
    char stock_symbol[10];
    int quantity;
    double price;
    time_t timestamp;
    struct Transaction* next;
} Transaction;

typedef struct PortfolioItem {
    char stock_symbol[10];
    int quantity;
    double avg_price;
    struct PortfolioItem* next;
} PortfolioItem;

// ================== Global Variables ==================

User* usersHashTable[HASH_TABLE_SIZE] = {NULL};
Stock* stockTree = NULL;
Transaction* buyQueue = NULL;
Transaction* sellStack = NULL;
PortfolioItem* currentPortfolio = NULL;
User* currentUser = NULL;

// ================== Function Prototypes ==================

// Stock management
Stock* createStock(const char* symbol, const char* name, double price, float priceChange,
    float high52Week, float low52Week, const char* marketCap,
    float peRatio, float dividendYield, float eps,
    const char* volume, const char* avgVolume,
    const char* overview, const char* analystRating);
Stock* insertStock(Stock* node, Stock* newStock);
Stock* searchStock(Stock* root, const char* symbol);
void displayStocks(Stock* root);
void initializeSampleStocks();
Stock* loadStocksFromFile(const char* filename);

// User management
unsigned int hash(const char* username);
int registerUser();
int loginUser();
void forgotPassword();
void logoutUser();
void displayAllUsers();
void resetUserPassword();
void loadUsersFromFile();
void saveUsersToFile();

// Transaction processing
void BuyOrder();
void SellOrder();
void processBuyOrders();
void processSellOrders();
void saveStockTransactions(User* user);
void loadStockTransactions(User* user);

// Portfolio management
void displayPortfolio();

// Money management
int creditUser(User* user, double amount, const char* description);
int debitUser(User* user, double amount, const char* description);
void displayBalance(User* user);
void displayTransactionHistory(User* user);

// File handling
void createDataDirectory();
void saveAllData();
void saveStockTree(Stock* root, FILE* file);
void saveUserTransactions(User* user);
void savePortfolio(User* user);
void loadAllData();
void loadUserTransactions(User* user);
void loadPortfolio(User* user);
void saveUserData();

// Utility functions
void clearInputBuffer();
void getHiddenPassword(char* prompt, char* buffer, size_t max_length);
unsigned long simpleHash(const char* str);
int isPasswordStrong(const char* pass);
char* getCurrentTime();

// Menu functions
void showLoginMenu();
void showAdminMenu();
void showUserMenu();
void adminMenu();
void userMenu();

// Admin functions
bool authenticateAdmin();
void createBackup();
bool addStock();
bool removeStock(const char* symbol);
void updateStockPrice(Stock* stockTree);
void showModifyStocksMenu();

// ================== Utility Functions ==================

void typewriter(const char* text, int delay_ms) {
    for (int i = 0; text[i] != '\0'; i++) {
        putchar(text[i]);
        fflush(stdout);
        usleep(delay_ms * 1000); 
    }
}
void clearScreen() {
    printf("\033[2J\033[H"); 
}

void setCLIStyle() {
    printf("%s%s", WHITE, BG_BLACK); 
    clearScreen();
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

unsigned long simpleHash(const char* str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }

    return hash;
}


void getHiddenPassword(char* prompt, char* buffer, size_t max_length) {
    int i = 0;
    int ch;

    printf("%s", prompt);
    fflush(stdout);

#ifdef _WIN32
    while (i < max_length - 1) {
        ch = _getch();  // Read character without echo

        if ((ch == 8 || ch == 127) && i > 0) {  // Backspace
            printf("\b \b");
            i--;
        } else if (ch == '\r' || ch == '\n') {  // Enter
            break;
        } else if (ch == 27 || ch == EOF) {  // Escape or Ctrl+D
            break;
        } else {
            buffer[i++] = ch;
            printf("*");
        }
    }
    buffer[i] = '\0';
    printf("\n");

#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < max_length - 1) {
        ch = getchar();

        if ((ch == 127 || ch == 8) && i > 0) {
            printf("\b \b");
            fflush(stdout);
            i--;
            continue;
        }

        if (ch == '\n' || ch == EOF) {
            break;
        }

        buffer[i++] = ch;
        printf("*");
        fflush(stdout);
    }

    buffer[i] = '\0';
    printf("\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}


int isPasswordStrong(const char* pass) {
    if (strlen(pass) < 8) {
        printf("Password must be at least 8 characters long.\n");
        return 0;
    }

    int hasUpper = 0, hasLower = 0, hasDigit = 0, hasSpecial = 0;
    for (int i = 0; pass[i] != '\0'; i++) {
        if (isupper(pass[i])) hasUpper = 1;
        else if (islower(pass[i])) hasLower = 1;
        else if (isdigit(pass[i])) hasDigit = 1;
        else if (!isalnum(pass[i])) hasSpecial = 1;
    }

    if (!hasUpper) {
        printf("Password must contain at least one uppercase letter.\n");
        return 0;
    }
    if (!hasLower) {
        printf("Password must contain at least one lowercase letter.\n");
        return 0;
    }
    if (!hasDigit) {
        printf("Password must contain at least one digit.\n");
        return 0;
    }
    if (!hasSpecial) {
        printf("Password must contain at least one special character.\n");
        return 0;
    }

    return 1;
}

char* getCurrentTime() {
    time_t now = time(NULL);
    return ctime(&now);
}

// ================== Stock Management ==================

Stock* createStock(const char* symbol, const char* name, double price, float priceChange,
    float high52Week, float low52Week, const char* marketCap,
    float peRatio, float dividendYield, float eps,
    const char* volume, const char* avgVolume,
    const char* overview, const char* analystRating)
{
    Stock* newStock = (Stock*)malloc(sizeof(Stock));
    if (newStock) {
        strncpy(newStock->symbol, symbol, sizeof(newStock->symbol) - 1);
        newStock->symbol[sizeof(newStock->symbol) - 1] = '\0';

        strncpy(newStock->name, name, sizeof(newStock->name) - 1);
        newStock->name[sizeof(newStock->name) - 1] = '\0';

        newStock->price = price;
        newStock->priceChange = priceChange;
        newStock->high52Week = high52Week;
        newStock->low52Week = low52Week;

        strncpy(newStock->marketCap, marketCap, sizeof(newStock->marketCap) - 1);
        newStock->marketCap[sizeof(newStock->marketCap) - 1] = '\0';

        newStock->peRatio = peRatio;
        newStock->dividendYield = dividendYield;
        newStock->eps = eps;

        strncpy(newStock->volume, volume, sizeof(newStock->volume) - 1);
        newStock->volume[sizeof(newStock->volume) - 1] = '\0';

        strncpy(newStock->avgVolume, avgVolume, sizeof(newStock->avgVolume) - 1);
        newStock->avgVolume[sizeof(newStock->avgVolume) - 1] = '\0';

        strncpy(newStock->overview, overview, sizeof(newStock->overview) - 1);
        newStock->overview[sizeof(newStock->overview) - 1] = '\0';

        strncpy(newStock->analystRating, analystRating, sizeof(newStock->analystRating) - 1);
        newStock->analystRating[sizeof(newStock->analystRating) - 1] = '\0';

        newStock->left = newStock->right = NULL;
        newStock->height = 1;
    }
    return newStock;
}

int getHeight(Stock* node) {
    return node ? node->height : 0;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int getBalance(Stock* node) {
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

Stock* rightRotate(Stock* y) {
    Stock* x = y->left;
    Stock* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    return x;
}

Stock* leftRotate(Stock* x) {
    Stock* y = x->right;
    Stock* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    return y;
}

Stock* insertStock(Stock* node, Stock* newStock) {
    if (!node) return newStock;

    int cmp = strcmp(newStock->symbol, node->symbol);
    if (cmp < 0)
        node->left = insertStock(node->left, newStock);
    else if (cmp > 0)
        node->right = insertStock(node->right, newStock);
    else {
        strcpy(node->name, newStock->name);
        node->price = newStock->price;
        node->priceChange = newStock->priceChange;
        node->high52Week = newStock->high52Week;
        node->low52Week = newStock->low52Week;
        strcpy(node->marketCap, newStock->marketCap);
        node->peRatio = newStock->peRatio;
        node->dividendYield = newStock->dividendYield;
        node->eps = newStock->eps;
        strcpy(node->volume, newStock->volume);
        strcpy(node->avgVolume, newStock->avgVolume);
        strcpy(node->overview, newStock->overview);
        strcpy(node->analystRating, newStock->analystRating);
        free(newStock);
        return node;
    }

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    if (balance > 1 && strcmp(newStock->symbol, node->left->symbol) < 0)
        return rightRotate(node);

    if (balance < -1 && strcmp(newStock->symbol, node->right->symbol) > 0)
        return leftRotate(node);

    if (balance > 1 && strcmp(newStock->symbol, node->left->symbol) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && strcmp(newStock->symbol, node->right->symbol) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

Stock* searchStock(Stock* root, const char* symbol) {
    if (!root || strcmp(root->symbol, symbol) == 0)
        return root;

    if (strcmp(symbol, root->symbol) < 0)
        return searchStock(root->left, symbol);

    return searchStock(root->right, symbol);
}

void displayStocks(Stock* root) {
    if (root) {
        displayStocks(root->left);
        
        printf(
            "Company Name         : %s\n"
            "Current Price        : ₹%.2f\n"
            "Stock Code           : %s\n"
            "Price Change         : %.2f%%\n"
            "52-Week High         : ₹%.2f\n"
            "52-Week Low          : ₹%.2f\n"
            "Market Capitalization: %s\n"
            "P/E Ratio            : %.2f\n"
            "Dividend Yield       : %.2f%%\n"
            "Earnings Per Share   : %.2f\n"
            "Volume               : %s\n"
            "Average Volume       : %s\n"
            "Company Overview     : %s\n"
            "Analyst Ratings      : %s\n"
            "------------------------------------------------------------\n",
            root->name, root->price, root->symbol, root->priceChange,
            root->high52Week, root->low52Week, root->marketCap,
            root->peRatio, root->dividendYield, root->eps,
            root->volume, root->avgVolume, root->overview, root->analystRating);
        
        displayStocks(root->right);
    }
}

Stock* loadStocksFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open stock file");
        return NULL;
    }

    Stock* root = NULL;
    char line[1024];
    char name[100] = "", symbol[10] = "", marketCap[20] = "";
    char volume[20] = "", avgVolume[20] = "", overview[256] = "", analystRating[20] = "";
    double price = 0.0;
    float priceChange = 0.0, high52Week = 0.0, low52Week = 0.0;
    float peRatio = 0.0, dividendYield = 0.0, eps = 0.0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Company Name", 12) == 0)
            sscanf(line, "Company Name       : %[^\n]", name);
        else if (strncmp(line, "Stock Code", 10) == 0)
            sscanf(line, "Stock Code         : %[^\n]", symbol);
        else if (strncmp(line, "Current Price", 13) == 0)
            sscanf(line, "Current Price      : ₹%lf", &price);  
        else if (strncmp(line, "Price Change", 12) == 0)
            sscanf(line, "Price Change       : %f%%", &priceChange);
        else if (strncmp(line, "52-Week High", 13) == 0)
            sscanf(line, "52-Week High       : ₹%f", &high52Week);
        else if (strncmp(line, "52-Week Low", 12) == 0)
            sscanf(line, "52-Week Low        : ₹%f", &low52Week);
        else if (strncmp(line, "Market Capitalization", 22) == 0)
            sscanf(line, "Market Capitalization: %[^\n]", marketCap);
        else if (strncmp(line, "P/E Ratio", 9) == 0)
            sscanf(line, "P/E Ratio          : %f", &peRatio);
        else if (strncmp(line, "Dividend Yield", 14) == 0)
            sscanf(line, "Dividend Yield     : %f%%", &dividendYield);
        else if (strncmp(line, "Earnings Per Share", 18) == 0)
            sscanf(line, "Earnings Per Share : %f", &eps);
        else if (strncmp(line, "Volume", 6) == 0 && strstr(line, "Average") == NULL)
            sscanf(line, "Volume             : %[^\n]", volume);
        else if (strncmp(line, "Average Volume", 14) == 0)
            sscanf(line, "Average Volume     : %[^\n]", avgVolume);
        else if (strncmp(line, "Company Overview", 16) == 0)
            sscanf(line, "Company Overview   : %[^\n]", overview);
        else if (strncmp(line, "Analyst Ratings", 15) == 0) {
            sscanf(line, "Analyst Ratings    : %[^\n]", analystRating);

            Stock* newStock = createStock(symbol, name, price, priceChange,
                                      high52Week, low52Week, marketCap,
                                      peRatio, dividendYield, eps,
                                      volume, avgVolume, overview, analystRating);
            root = insertStock(root, newStock);
        }
    }

    fclose(file);
    return root;
}

void initializeSampleStocks() {
    stockTree = loadStocksFromFile(STOCK_FILE);
}

// ================== User Authentication ==================

unsigned int hash(const char* username) {
    unsigned int hash = 0;
    for (int i = 0; username[i] != '\0'; i++) {
        hash = 31 * hash + username[i];
    }
    return hash % HASH_TABLE_SIZE;
}

void loadUsersFromFile() {
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR REGISTRATION_FILE);
    
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        User* newUser = (User*)malloc(sizeof(User));
        if (!newUser) continue;

        char* token = strtok(line, "|");
        if (token) strncpy(newUser->username, token, sizeof(newUser->username));
        
        token = strtok(NULL, "|");
        if (token) strncpy(newUser->email, token, sizeof(newUser->email));
        
        token = strtok(NULL, "|");
        if (token) strncpy(newUser->password, token, sizeof(newUser->password));
        
        token = strtok(NULL, "|");
        if (token) newUser->balance = atof(token);
        
        token = strtok(NULL, "|");
        if (token) newUser->failed_attempts = atoi(token);
        
        token = strtok(NULL, "|");
        if (token) newUser->last_login = (time_t)atol(token);
        
        token = strtok(NULL, "|");
        if (token) strncpy(newUser->role, token, sizeof(newUser->role));
        
        newUser->transactions = NULL;
        newUser->stockTransactions = NULL;
        
        unsigned int index = hash(newUser->username);
        newUser->next = usersHashTable[index];
        usersHashTable[index] = newUser;
    }
    fclose(file);
}

void saveUsersToFile() {
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR REGISTRATION_FILE);
    
    FILE* file = fopen(filename, "w");
    if (!file) return;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        User* user = usersHashTable[i];
        while (user) {
            fprintf(file, "%s|%s|%s|%.2f|%d|%ld|%s\n",
                   user->username,
                   user->email,
                   user->password,
                   user->balance,
                   user->failed_attempts,
                   user->last_login,
                   user->role);
            user = user->next;
        }
    }
    fclose(file);
}

int registerUser() {
    User* newUser = (User*)malloc(sizeof(User));
    if (!newUser) {
        printf("Memory allocation failed!\n");
        return 0;
    }

    printf("Enter username: ");
    if (scanf("%49s", newUser->username) != 1) {
        clearInputBuffer();
        free(newUser);
        printf("Invalid username input.\n");
        return 0;
    }
    clearInputBuffer();
    
    unsigned int index = hash(newUser->username);
    User* temp = usersHashTable[index];
    while (temp) {
        if (strcmp(temp->username, newUser->username) == 0) {
            printf("Username already exists!\n");
            free(newUser);
            return 0;
        }
        temp = temp->next;
    }

    printf("Enter email: ");
    if (scanf("%99s", newUser->email) != 1) {
        clearInputBuffer();
        free(newUser);
        printf("Invalid email input.\n");
        return 0;
    }
    clearInputBuffer();
    
    char password[100], confirm[100];
    int passwordValid = 0;
    do {
        do {
            getHiddenPassword("Enter password (min 8 chars, upper/lower case, digit, special): ", 
                            password, sizeof(password));
            passwordValid = isPasswordStrong(password);
            if (!passwordValid) {
                printf("Password doesn't meet strength requirements.\n");
            }
        } while (!passwordValid);

        getHiddenPassword("Confirm password: ", confirm, sizeof(confirm));
        
        if (strcmp(password, confirm) != 0) {
            printf("Passwords don't match! Try again.\n");
        }
    } while (strcmp(password, confirm) != 0);

    unsigned long hashed = simpleHash(password);
    snprintf(newUser->password, sizeof(newUser->password), "%lu", hashed);
    
    char roleChoice;
    printf("Register as (a)admin or (u)user? [a/u]: ");
    scanf(" %c", &roleChoice);
    clearInputBuffer();

    if (tolower(roleChoice) == 'a') {
        char adminKey[100];
        printf("Enter admin registration key: ");
        getHiddenPassword("", adminKey, sizeof(adminKey));
        
        if (strcmp(adminKey, ADMIN_REGISTRATION_KEY) != 0) {
            printf("Invalid admin key! Registering as regular user.\n");
            strcpy(newUser->role, "user");
        } else {
            strcpy(newUser->role, "admin");
            printf("Admin registration successful!\n");
        }
    } else {
        strcpy(newUser->role, "user");
    }
    
    newUser->failed_attempts = 0;
    newUser->last_login = time(NULL);
    newUser->balance = 0.0;
    newUser->transactions = NULL;
    newUser->stockTransactions = NULL;
    newUser->next = usersHashTable[index];
    usersHashTable[index] = newUser;

    saveUsersToFile();
    printf("Registration successful! Role: %s\n", newUser->role);
    loadPortfolio(newUser);
    currentPortfolio = NULL;
    return 1;
}

int loginUser() {
    char username[50], password[100];
    
    printf("Enter username: ");
    if (scanf("%49s", username) != 1) {
        clearInputBuffer();
        printf("Invalid username input.\n");
        return 0;
    }
    clearInputBuffer();
    
    getHiddenPassword("Enter password: ", password, sizeof(password));
    
    unsigned int index = hash(username);
    User* user = usersHashTable[index];
    
    while (user) {
        if (strcmp(user->username, username) == 0) {
            if (user->failed_attempts >= MAX_LOGIN_ATTEMPTS) {
                printf("Account locked due to too many failed attempts.\n");
                return 0;
            }
            
            unsigned long hashedInput = simpleHash(password);
            char hashedStr[20];
            snprintf(hashedStr, sizeof(hashedStr), "%lu", hashedInput);
            
            if (strcmp(user->password, hashedStr) == 0) {
                user->failed_attempts = 0;
                user->last_login = time(NULL);
                currentUser = user;
                loadUserTransactions(currentUser);
                currentPortfolio = NULL; 
                loadPortfolio(currentUser);
                loadStockTransactions(currentUser);
                printf("Login successful! Welcome, %s (%s)\n", username, user->role);
                saveUsersToFile();
                return 1;
            } else {
                user->failed_attempts++;
                printf("Invalid password. Attempts remaining: %d\n", 
                      MAX_LOGIN_ATTEMPTS - user->failed_attempts);
                saveUsersToFile();
                return 0;
            }
        }
        user = user->next;
    }
    
    printf("User not found.\n");
    return 0;
}



void forgotPassword() {
    char username[50], email[100];
    printf("Enter username: ");
    scanf("%49s", username);
    clearInputBuffer();  

    printf("Enter email: ");
    scanf("%99s", email);
    clearInputBuffer();  

    unsigned int index = hash(username);
    User* user = usersHashTable[index];
    
    while (user) {
        if (strcmp(user->username, username) == 0 && 
            strcmp(user->email, email) == 0) {
            char token[20];
            srand(time(NULL));
            snprintf(token, sizeof(token), "%d", rand() % 1000000 + 100000);
            
            printf("Password reset token sent to %s: %s\n", email, token);
            printf("Would you like to reset now? (y/n): ");
            
            char choice;
            scanf(" %c", &choice);
            clearInputBuffer();  

            if (tolower(choice) == 'y') {
                char newPass[100], confirm[100];
                int match = 0;
                do {
                    getHiddenPassword("New password: ", newPass, sizeof(newPass));
                    getHiddenPassword("Confirm password: ", confirm, sizeof(confirm));
                    
                    match = strcmp(newPass, confirm);
                    if (match != 0) {
                        printf("Passwords don't match! Please try again.\n");
                    }
                } while (match != 0);
                
                unsigned long hashed = simpleHash(newPass);
                snprintf(user->password, sizeof(user->password), "%lu", hashed);
                user->failed_attempts = 0;
                printf("Password updated successfully!\n");
                saveUsersToFile();
            }
            return;
        }
        user = user->next;
    }
    
    printf("No matching username and email found.\n");
}

void logoutUser() {
    if (currentUser) {
        saveUserData();
        saveStockTransactions(currentUser);
    }
    PortfolioItem* temp;
    while (currentPortfolio) {
        temp = currentPortfolio;
        currentPortfolio = currentPortfolio->next;
        free(temp);
    }

    currentPortfolio = NULL;
    currentUser = NULL;

    printf("Logged out successfully.\n");
}


// ================== Money Management ==================

int creditUser(User* user, double amount, const char* description) {
    if (!user || amount <= 0) {
        printf("Invalid credit operation.\n");
        return 0;
    }
    
    user->balance += amount;
    
    MoneyTransaction* newTrans = (MoneyTransaction*)malloc(sizeof(MoneyTransaction));
    if (!newTrans) {
        printf("Failed to record transaction.\n");
        return 0;
    }
    
    strncpy(newTrans->type, "credit", sizeof(newTrans->type));
    newTrans->amount = amount;
    newTrans->balance_after = user->balance;
    strncpy(newTrans->description, description, sizeof(newTrans->description));
    newTrans->timestamp = time(NULL);
    newTrans->next = user->transactions;
    user->transactions = newTrans;
    
    printf("Credited ₹%.2f to account. New balance: ₹%.2f\n", amount, user->balance);
    saveUserData();
    return 1;
}

int debitUser(User* user, double amount, const char* description) {
    if (!user || amount <= 0) {
        printf("Invalid debit operation.\n");
        return 0;
    }
    
    if (amount > user->balance) {
        printf("Insufficient funds. Current balance: ₹%.2f\n", user->balance);
        return 0;
    }
    
    user->balance -= amount;
    
    MoneyTransaction* newTrans = (MoneyTransaction*)malloc(sizeof(MoneyTransaction));
    if (!newTrans) {
        printf("Failed to record transaction.\n");
        return 0;
    }
    
    strncpy(newTrans->type, "debit", sizeof(newTrans->type));
    newTrans->amount = amount;
    newTrans->balance_after = user->balance;
    strncpy(newTrans->description, description, sizeof(newTrans->description));
    newTrans->timestamp = time(NULL);
    newTrans->next = user->transactions;
    user->transactions = newTrans;
    
    printf("Debited ₹%.2f from account. New balance: ₹%.2f\n", amount, user->balance);
    saveUserData();
    return 1;
}

void displayBalance(User* user) {
    if (!user) {
        printf("No user logged in.\n");
        return;
    }
    printf("\n=== Account Balance ===\n");
    printf("Current balance: ₹%.2f\n", user->balance);
}

void displayTransactionHistory(User* user) {
    if (!user) {
        printf("No user logged in.\n");
        return;
    }
    
    printf("\n=== Transaction History ===\n");
    if (!user->transactions) {
        printf("No transactions found.\n");
        return;
    }
    
    printf("%-20s %-10s %-10s %-15s %s\n", 
           "Date", "Type", "Amount", "Balance", "Description");
    
    MoneyTransaction* trans = user->transactions;
    while (trans) {
        char dateStr[20];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", localtime(&trans->timestamp));
        
        printf("%-20s %-10s ₹%-9.2f ₹%-14.2f %s\n", 
               dateStr, trans->type, trans->amount, trans->balance_after, trans->description);
        trans = trans->next;
    }
}

// ================== Transaction Processing ==================

void saveStockTransactions(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" USER_STOCK_FILE, user->username);
    
    FILE* file = fopen(filename, "w");
    if (!file) return;

    StockTransaction* trans = user->stockTransactions;
    while (trans) {
        fprintf(file, "%s|%s|%d|%.2f|%.2f|%s|%ld\n",
               trans->type,
               trans->stock_symbol,
               trans->quantity,
               trans->price,
               trans->total_amount,
               trans->status,
               trans->timestamp);
        trans = trans->next;
    }
    fclose(file);
}

void loadStockTransactions(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" USER_STOCK_FILE, user->username);
    
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[256];
    StockTransaction* last = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        StockTransaction* newTrans = (StockTransaction*)malloc(sizeof(StockTransaction));
        if (!newTrans) break;
        
        if (sscanf(line, "%[^|]|%[^|]|%d|%lf|%lf|%[^|]|%ld",
            newTrans->type,
            newTrans->stock_symbol,
            &newTrans->quantity,
            &newTrans->price,
            &newTrans->total_amount,
            newTrans->status,
            &newTrans->timestamp) == 7) {
            
            newTrans->next = NULL;
            
            if (!user->stockTransactions) {
                user->stockTransactions = newTrans;
            } else {
                last->next = newTrans;
            }
            last = newTrans;
        } else {
            free(newTrans);
        }
    }
    fclose(file);
}

void trim(char* str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return; 


    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
}

void BuyOrder() {
    if (!currentUser) {
        printf("\nPlease login first.\n");
        return;
    }

    printf("\n=== Available Stocks ===\n");
    printf("+-----+------------+--------------------------------+-----------+\n");
    printf("| No. | Symbol     | Company Name                   | Price     |\n");
    printf("+-----+------------+--------------------------------+-----------+\n");

    typedef struct {
        char symbol[20];
        char name[100];
        double price;
    } DisplayStock;

    DisplayStock stocks[100];
    int stockCount = 0;

    FILE* file = fopen("stock_display.txt", "r");
    if (!file) {
        printf("| Unable to load stock information                     |\n");
        printf("+-----------------------------------------------------+\n");
        return;
    }

    char line[256];
    fgets(line, sizeof(line), file); // Skip header

    while (fgets(line, sizeof(line), file) && stockCount < 100) {
        int pk;
        char company[100], symbol[20], priceStr[20];
        if (sscanf(line, "%d | %[^|]| %[^|]| ₹%[^\n]", &pk, company, symbol, priceStr) == 4) {
            trim(symbol);
            trim(company);
            strncpy(stocks[stockCount].symbol, symbol, sizeof(stocks[0].symbol) - 1);
            strncpy(stocks[stockCount].name, company, sizeof(stocks[0].name) - 1);
            stocks[stockCount].price = atof(priceStr);

            printf("| %-3d | %-10s | %-30s | ₹%-8.2f |\n",
                   stockCount + 1, symbol, company, stocks[stockCount].price);
            stockCount++;
        }
    }
    fclose(file);

    printf("+-----+------------+--------------------------------+-----------+\n");

    if (stockCount == 0) {
        printf("No stocks available for trading.\n");
        return;
    }

    int choice;
    printf("\nEnter stock number to buy (1-%d) or 0 to cancel: ", stockCount);
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > stockCount) {
        clearInputBuffer();
        printf("Invalid selection.\n");
        return;
    }
    clearInputBuffer();

    DisplayStock selected = stocks[choice - 1];

    char cleanSymbol[20];
    strncpy(cleanSymbol, selected.symbol, sizeof(cleanSymbol));
    trim(cleanSymbol);

    Stock* stock = searchStock(stockTree, cleanSymbol);
    if (!stock) {
        printf("\n Sorry, full stock details not found for '%s'.\n", cleanSymbol);
        return;
    }

    printf("\n=== Stock Details ===\n");
    printf("Company: %s\n", stock->name);
    printf("Symbol: %s\n", stock->symbol);
    printf("Current Price: ₹%.2f\n", selected.price);
    printf("52-Week Range: ₹%.2f - ₹%.2f\n", stock->low52Week, stock->high52Week);
    printf("Market Cap: %s\n", stock->marketCap);
    printf("P/E Ratio: %.2f\n", stock->peRatio);

    PortfolioItem* item = currentPortfolio;
    while (item) {
        if (strcmp(item->stock_symbol, selected.symbol) == 0) {
            printf("\nYou currently own %d shares (Avg price: ₹%.2f)\n",
                   item->quantity, item->avg_price);
            break;
        }
        item = item->next;
    }

    int quantity;
    printf("\nEnter quantity to buy: ");
    if (scanf("%d", &quantity) != 1 || quantity <= 0) {
        clearInputBuffer();
        printf("Invalid quantity.\n");
        return;
    }
    clearInputBuffer();

    double totalCost = quantity * selected.price;
    printf("\nOrder Summary:\n");
    printf("Stock: %s (%s)\n", stock->name, stock->symbol);
    printf("Quantity: %d\n", quantity);
    printf("Price per share: ₹%.2f\n", selected.price);
    printf("Total Cost: ₹%.2f\n", totalCost);
    printf("Your Balance: ₹%.2f\n", currentUser->balance);

    printf("\nConfirm purchase? (y/n): ");
    char confirm;
    scanf(" %c", &confirm);
    clearInputBuffer();

    if (tolower(confirm) != 'y') {
        printf("Purchase cancelled.\n");
        return;
    }

    if (totalCost > currentUser->balance) {
        printf("Insufficient funds. Needed: ₹%.2f, Available: ₹%.2f\n",
               totalCost, currentUser->balance);
        return;
    }

    printf("\nProcessing your order");
    fflush(stdout);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        SLEEP(1);
    }
    printf("\n");

    if (!debitUser(currentUser, totalCost, "Stock purchase")) {
        printf("Payment processing failed!\n");
        return;
    }

    item = currentPortfolio;
    int found = 0;

    while (item) {
        if (strcmp(item->stock_symbol, selected.symbol) == 0) {
            double newTotal = (item->avg_price * item->quantity) + totalCost;
            item->quantity += quantity;
            item->avg_price = newTotal / item->quantity;
            found = 1;
            break;
        }
        item = item->next;
    }

    if (!found) {
        PortfolioItem* newItem = malloc(sizeof(PortfolioItem));
        if (!newItem) {
            printf("Memory allocation failed for portfolio.\n");
            return;
        }
        strcpy(newItem->stock_symbol, selected.symbol);
        newItem->quantity = quantity;
        newItem->avg_price = selected.price;
        newItem->next = currentPortfolio;
        currentPortfolio = newItem;
    }

    StockTransaction* newTrans = malloc(sizeof(StockTransaction));
    if (!newTrans) {
        printf("Memory allocation failed for transaction.\n");
        return;
    }

    strcpy(newTrans->type, "BUY");
    strcpy(newTrans->stock_symbol, selected.symbol);
    newTrans->quantity = quantity;
    newTrans->price = selected.price;
    newTrans->total_amount = totalCost;
    strcpy(newTrans->status, "COMPLETED");
    newTrans->timestamp = time(NULL);
    newTrans->next = currentUser->stockTransactions;
    currentUser->stockTransactions = newTrans;

    savePortfolio(currentUser);
    saveStockTransactions(currentUser);
    saveUserData();

    printf("\n=== Transaction Successful ===\n");
    printf("Purchased %d shares of %s\n", quantity, stock->symbol);
    printf("At ₹%.2f per share\n", selected.price);
    printf("Total Amount: ₹%.2f\n", totalCost);
    printf("New Balance: ₹%.2f\n", currentUser->balance);
    printf("Transaction Time: %s", ctime(&newTrans->timestamp));
}


void SellOrder() {
    if (!currentUser) {
        printf("No user logged in.\n");
        return;
    }
    PortfolioItem* item = currentPortfolio;
    PortfolioItem* list[100];
    int count = 0;

    printf("\n╔══════════════════════════════════════╗\n");
    printf("║          YOUR STOCK PORTFOLIO        ║\n");
    printf("╠═════╦════════════╦═════════╦════════╣\n");
    printf("║ No. ║ Stock      ║ Shares  ║ Avg.   ║\n");
    printf("║     ║ Symbol     ║ Owned   ║ Price  ║\n");
    printf("╠═════╬════════════╬═════════╬════════╣\n");

    while (item) {
        if (item->quantity > 0) {
            list[count] = item;
            printf("║ %-3d ║ %-10s ║ %-7d ║ ₹%-7.2f║\n", count + 1, item->stock_symbol, item->quantity, item->avg_price);
            count++;
        }
        item = item->next;
    }

    if (count == 0) {
        printf("║        No stocks in portfolio        ║\n");
        printf("╚═════╩════════════╩═════════╩════════╝\n");
        return;
    }
    printf("╚═════╩════════════╩═════════╩════════╝\n");

    // 2. Let User Select
    int choice;
    printf("\nEnter stock number to sell (1-%d) or 0 to cancel: ", count);
    scanf("%d", &choice);
    while (getchar() != '\n');

    if (choice < 1 || choice > count) {
        printf("Transaction cancelled.\n");
        return;
    }

    PortfolioItem* selected = list[choice - 1];
    const char* symbol = selected->stock_symbol;
    printf("\nYou own %d shares of %s.\n", selected->quantity, symbol);
    int quantity;
    printf("Enter quantity to sell: ");
    scanf("%d", &quantity);
    while (getchar() != '\n');

    if (quantity <= 0 || quantity > selected->quantity) {
        printf("Invalid quantity.\n");
        return;
    }
    int opt;
    double sellPrice;
    printf("\nChoose selling option:\n");
    printf("1. Sell at current market price\n");
    printf("2. Sell at a custom price\n");
    printf("Enter your choice: ");
    scanf("%d", &opt);
    while (getchar() != '\n');

    Stock* stock = searchStock(stockTree, symbol);
    if (!stock) {
        printf("Stock not found.\n");
        return;
    }

    if (opt == 1) {
        sellPrice = stock->price;
    } else if (opt == 2) {
        printf("Enter your desired selling price per share: ");
        scanf("%lf", &sellPrice);
        while (getchar() != '\n');
        if (sellPrice <= 0) {
            printf("Invalid price.\n");
            return;
        }
    } else {
        printf("Invalid option.\n");
        return;
    }
    printf("\nProcessing sale of %d shares of %s at ₹%.2f", quantity, symbol, sellPrice);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        SLEEP(1);
    }
    printf("\n");

    selected->quantity -= quantity;
    if (selected->quantity == 0) {
        PortfolioItem* curr = currentPortfolio;
        PortfolioItem* prev = NULL;
        while (curr) {
            if (curr == selected) {
                if (prev) prev->next = curr->next;
                else currentPortfolio = curr->next;
                free(curr);
                break;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    double totalValue = quantity * sellPrice;
    currentUser->balance += totalValue;
    StockTransaction* newTrans = malloc(sizeof(StockTransaction));
    if (newTrans) {
        strcpy(newTrans->type, "SELL");
        strcpy(newTrans->stock_symbol, symbol);
        newTrans->quantity = quantity;
        newTrans->price = sellPrice;
        newTrans->total_amount = totalValue;
        strcpy(newTrans->status, "COMPLETED");
        newTrans->timestamp = time(NULL);
        newTrans->next = currentUser->stockTransactions;
        currentUser->stockTransactions = newTrans;
    }

    printf("\n Sale successful! ₹%.2f added to your balance.\n", totalValue);
    printf(" New Balance: ₹%.2f\n", currentUser->balance);

    savePortfolio(currentUser);
    saveStockTransactions(currentUser);
    saveUserData();
}


void processBuyOrders() {
    if (!buyQueue) {
        printf("No buy orders to process.\n");
        return;
    }

    printf("\n=== Processing Buy Orders ===\n");
    Transaction* current = buyQueue;
    while (current) {
        Stock* stock = searchStock(stockTree, current->stock_symbol);
        if (!stock) {
            printf("Stock %s not found!\n", current->stock_symbol);
            current = current->next;
            continue;
        }

        if (current->price >= stock->price) {
            double totalCost = current->quantity * stock->price;
            
            if (debitUser(currentUser, totalCost, "Stock purchase")) {
                PortfolioItem* item = currentPortfolio;
                PortfolioItem* prev = NULL;
                
                while (item) {
                    if (strcmp(item->stock_symbol, current->stock_symbol) == 0) {
                        double newTotal = (item->avg_price * item->quantity) + (stock->price * current->quantity);
                        item->quantity += current->quantity;
                        item->avg_price = newTotal / item->quantity;
                        break;
                    }
                    prev = item;
                    item = item->next;
                }
                
                if (!item) {
                    PortfolioItem* newItem = (PortfolioItem*)malloc(sizeof(PortfolioItem));
                    if (newItem) {
                        strcpy(newItem->stock_symbol, current->stock_symbol);
                        newItem->quantity = current->quantity;
                        newItem->avg_price = stock->price;
                        newItem->next = currentPortfolio;
                        currentPortfolio = newItem;
                    }
                }
                
                // Update transaction status
                StockTransaction* trans = currentUser->stockTransactions;
                while (trans) {
                    if (strcmp(trans->stock_symbol, current->stock_symbol) == 0 &&
                        strcmp(trans->type, "BUY") == 0 &&
                        trans->status[0] == 'P') { // PENDING
                        strcpy(trans->status, "COMPLETED");
                        trans->price = stock->price;
                        trans->total_amount = trans->quantity * stock->price;
                        break;
                    }
                    trans = trans->next;
                }
                
                printf("Processed buy order: %d shares of %s at ₹%.2f\n", 
                      current->quantity, current->stock_symbol, stock->price);
            }
        } else {
            printf("Buy order price too low for %s (order: ₹%.2f, market: ₹%.2f)\n",
                  current->stock_symbol, current->price, stock->price);
        }
        
        current = current->next;
    }
    
    // Clear processed orders
    while (buyQueue) {
        Transaction* temp = buyQueue;
        buyQueue = buyQueue->next;
        free(temp);
    }
    
    savePortfolio(currentUser);
    saveStockTransactions(currentUser);
}

void processSellOrders() {
    if (!sellStack) {
        printf("No sell orders to process.\n");
        return;
    }

    printf("\n=== Processing Sell Orders ===\n");
    Transaction* current = sellStack;
    while (current) {
        Stock* stock = searchStock(stockTree, current->stock_symbol);
        if (!stock) {
            printf("Stock %s not found!\n", current->stock_symbol);
            current = current->next;
            continue;
        }

        // Check if user has the stock in portfolio
        PortfolioItem* item = currentPortfolio;
        PortfolioItem* prev = NULL;
        int found = 0;
        
        while (item) {
            if (strcmp(item->stock_symbol, current->stock_symbol) == 0) {
                found = 1;
                if (item->quantity >= current->quantity) {
                    double totalValue = current->quantity * stock->price;
                    
                    if (creditUser(currentUser, totalValue, "Stock sale")) {
                        item->quantity -= current->quantity;
                        
                        if (item->quantity == 0) {
                            if (prev) {
                                prev->next = item->next;
                            } else {
                                currentPortfolio = item->next;
                            }
                            free(item);
                        }
                        
                        // Update transaction status
                        StockTransaction* trans = currentUser->stockTransactions;
                        while (trans) {
                            if (strcmp(trans->stock_symbol, current->stock_symbol) == 0 &&
                                strcmp(trans->type, "SELL") == 0 &&
                                trans->status[0] == 'P') { 
                                strcpy(trans->status, "COMPLETED");
                                trans->price = stock->price;
                                trans->total_amount = trans->quantity * stock->price;
                                break;
                            }
                            trans = trans->next;
                        }
                        
                        printf("Processed sell order: %d shares of %s at ₹%.2f\n", 
                              current->quantity, current->stock_symbol, stock->price);
                    }
                } else {
                    printf("Insufficient shares of %s to sell (have %d, need %d)\n",
                          current->stock_symbol, item->quantity, current->quantity);
                }
                break;
            }
            prev = item;
            item = item->next;
        }
        
        if (!found) {
            printf("You don't own any shares of %s\n", current->stock_symbol);
        }
        
        current = current->next;
    }
    
    // Clear processed orders
    while (sellStack) {
        Transaction* temp = sellStack;
        sellStack = sellStack->next;
        free(temp);
    }
    
    savePortfolio(currentUser);
    saveStockTransactions(currentUser);
}

// ================== Portfolio Management ==================

void displayPortfolio() {
    if (!currentUser) {
        printf("No user logged in.\n");
        return;
    }

    if (!currentPortfolio) {
        printf("\nYour portfolio is empty.\n");
        return;
    }

    printf("\n=== Investment Portfolio ===\n");
    printf("+------------+------------+---------------+---------------+----------------+\n");
    printf("| Symbol     | Quantity   | Avg. Price    | Current Price | Current Value  |\n");
    printf("+------------+------------+---------------+---------------+----------------+\n");

    PortfolioItem* item = currentPortfolio;
    double totalValue = 0.0;

    while (item) {
        Stock* stock = searchStock(stockTree, item->stock_symbol);
        double currentPrice = stock ? stock->price : 0.0;
        double currentValue = item->quantity * currentPrice;
        totalValue += currentValue;

        printf("| %-10s | %-10d | ₹%-12.2f | ₹%-12.2f | ₹%-14.2f |\n",
               item->stock_symbol,
               item->quantity,
               item->avg_price,
               currentPrice,
               currentValue);

        item = item->next;
    }

    printf("+------------+------------+---------------+---------------+----------------+\n");
    printf("Total Portfolio Value: ₹%.2f\n", totalValue);
}

void savePortfolio(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" PORTFOLIO_FILE, user->username);
    
    FILE* file = fopen(filename, "w");
    if (!file) return;

    PortfolioItem* item = currentPortfolio;
    while (item) {
        fprintf(file, "%s|%d|%.2f\n",
               item->stock_symbol,
               item->quantity,
               item->avg_price);
        item = item->next;
    }
    fclose(file);
}

void loadPortfolio(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" PORTFOLIO_FILE, user->username);
    
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[100];
    PortfolioItem* last = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        PortfolioItem* newItem = (PortfolioItem*)malloc(sizeof(PortfolioItem));
        if (!newItem) break;
        
        if (sscanf(line, "%[^|]|%d|%lf",
            newItem->stock_symbol,
            &newItem->quantity,
            &newItem->avg_price) == 3) {
            
            newItem->next = NULL;
            
            if (!currentPortfolio) {
                currentPortfolio = newItem;
            } else {
                last->next = newItem;
            }
            last = newItem;
        } else {
            free(newItem);
        }
    }
    fclose(file);
}

// ================== File Handling ==================

void createDataDirectory() {
    struct stat st = {0};
    if (stat(USER_DATA_DIR, &st) == -1) {
        mkdir(USER_DATA_DIR, 0700);
    }
}

void saveAllData() {
    saveUsersToFile();
    
    // Save stock 
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR STOCK_FILE);
    FILE* file = fopen(filename, "w");
    if (file) {
        saveStockTree(stockTree, file);
        fclose(file);
    }
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        User* user = usersHashTable[i];
        while (user) {
            saveUserTransactions(user);
            saveStockTransactions(user);
            user = user->next;
        }
    }
}

void saveStockTree(Stock* root, FILE* file) {
    if (root) {
        saveStockTree(root->left, file);
        fprintf(file, "Company Name         : %s\n", root->name);
        fprintf(file, "Stock Code           : %s\n", root->symbol);
        fprintf(file, "Current Price        : ₹%.2f\n", root->price);
        fprintf(file, "Price Change         : %.2f%%\n", root->priceChange);
        fprintf(file, "52-Week High         : ₹%.2f\n", root->high52Week);
        fprintf(file, "52-Week Low          : ₹%.2f\n", root->low52Week);
        fprintf(file, "Market Capitalization: %s\n", root->marketCap);
        fprintf(file, "P/E Ratio            : %.2f\n", root->peRatio);
        fprintf(file, "Dividend Yield       : %.2f%%\n", root->dividendYield);
        fprintf(file, "Earnings Per Share   : %.2f\n", root->eps);
        fprintf(file, "Volume               : %s\n", root->volume);
        fprintf(file, "Average Volume       : %s\n", root->avgVolume);
        fprintf(file, "Company Overview     : %s\n", root->overview);
        fprintf(file, "Analyst Ratings      : %s\n", root->analystRating);
        fprintf(file, "------------------------------------------------------------\n");
        saveStockTree(root->right, file);
    }
}

void saveUserTransactions(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" TRANSACTION_FILE, user->username);
    
    FILE* file = fopen(filename, "w");
    if (!file) return;

    MoneyTransaction* trans = user->transactions;
    while (trans) {
        fprintf(file, "%s|%.2f|%.2f|%s|%ld\n",
               trans->type,
               trans->amount,
               trans->balance_after,
               trans->description,
               trans->timestamp);
        trans = trans->next;
    }
    fclose(file);
}

void loadUserTransactions(User* user) {
    if (!user) return;
    
    char filename[100];
    snprintf(filename, sizeof(filename), USER_DATA_DIR "%s_" TRANSACTION_FILE, user->username);
    
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[256];
    MoneyTransaction* last = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        MoneyTransaction* newTrans = (MoneyTransaction*)malloc(sizeof(MoneyTransaction));
        if (!newTrans) break;
        
        if (sscanf(line, "%[^|]|%lf|%lf|%[^|]|%ld",
            newTrans->type,
            &newTrans->amount,
            &newTrans->balance_after,
            newTrans->description,
            &newTrans->timestamp) == 5) {
            
            newTrans->next = NULL;
            
            if (!user->transactions) {
                user->transactions = newTrans;
            } else {
                last->next = newTrans;
            }
            last = newTrans;
        } else {
            free(newTrans);
        }
    }
    fclose(file);
}

void loadAllData() {
    createDataDirectory();
    loadUsersFromFile();
    stockTree = loadStocksFromFile(STOCK_FILE);
}

void saveUserData() {
    if (!currentUser) return;
    
    saveUsersToFile();
    saveUserTransactions(currentUser);
    savePortfolio(currentUser);
    saveStockTransactions(currentUser);
}

// ================== Admin Functions ==================

bool authenticateAdmin() {
    char username[50], password[100], secretKey[100];
    int attempts = 0;
    
    while (attempts < MAX_ATTEMPTS) {
        printf("Admin Username: ");
        scanf("%49s", username);
        clearInputBuffer();
        
        getHiddenPassword("Admin Password: ", password, sizeof(password));
        getHiddenPassword("Admin Secret Key: ", secretKey, sizeof(secretKey));
        
        if (strcmp(username, ADMIN_USERNAME) == 0 &&
            strcmp(password, ADMIN_PASSWORD) == 0 &&
            strcmp(secretKey, ADMIN_SECRET_KEY) == 0) {
            return true;
        }
        
        attempts++;
        printf("Authentication failed. Attempts remaining: %d\n", MAX_ATTEMPTS - attempts);
    }
    
    printf("Maximum attempts reached. Access denied.\n");
    return false;
}

void createBackup() {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char backupDir[100];
    strftime(backupDir, sizeof(backupDir), USER_DATA_DIR "backup_%Y%m%d_%H%M%S", tm);
    
    mkdir(backupDir, 0700);
    
    char command[256];
    snprintf(command, sizeof(command), "cp -r " USER_DATA_DIR "*.* %s/", backupDir);
    system(command);
    
    printf("Backup created successfully at %s\n", backupDir);
}

bool addStock() {
    Stock newStock;
    printf("\n=== Add New Stock ===\n");
    
    printf("Stock Symbol (e.g., AAPL): ");
    scanf("%9s", newStock.symbol);
    clearInputBuffer();
    
    if (searchStock(stockTree, newStock.symbol)) {
        printf("Stock with symbol %s already exists!\n", newStock.symbol);
        return false;
    }
    
    printf("Company Name: ");
    fgets(newStock.name, sizeof(newStock.name), stdin);
    newStock.name[strcspn(newStock.name, "\n")] = '\0';
    
    printf("Current Price: ");
    scanf("%lf", &newStock.price);
    clearInputBuffer();
    
    printf("Price Change (%%): ");
    scanf("%f", &newStock.priceChange);
    clearInputBuffer();
    
    printf("52-Week High: ");
    scanf("%f", &newStock.high52Week);
    clearInputBuffer();
    
    printf("52-Week Low: ");
    scanf("%f", &newStock.low52Week);
    clearInputBuffer();
    
    printf("Market Capitalization: ");
    fgets(newStock.marketCap, sizeof(newStock.marketCap), stdin);
    newStock.marketCap[strcspn(newStock.marketCap, "\n")] = '\0';
    
    printf("P/E Ratio: ");
    scanf("%f", &newStock.peRatio);
    clearInputBuffer();
    
    printf("Dividend Yield (%%): ");
    scanf("%f", &newStock.dividendYield);
    clearInputBuffer();
    
    printf("Earnings Per Share: ");
    scanf("%f", &newStock.eps);
    clearInputBuffer();
    
    printf("Volume: ");
    fgets(newStock.volume, sizeof(newStock.volume), stdin);
    newStock.volume[strcspn(newStock.volume, "\n")] = '\0';
    
    printf("Average Volume: ");
    fgets(newStock.avgVolume, sizeof(newStock.avgVolume), stdin);
    newStock.avgVolume[strcspn(newStock.avgVolume, "\n")] = '\0';
    
    printf("Company Overview: ");
    fgets(newStock.overview, sizeof(newStock.overview), stdin);
    newStock.overview[strcspn(newStock.overview, "\n")] = '\0';
    
    printf("Analyst Rating: ");
    fgets(newStock.analystRating, sizeof(newStock.analystRating), stdin);
    newStock.analystRating[strcspn(newStock.analystRating, "\n")] = '\0';
    
    Stock* stock = createStock(newStock.symbol, newStock.name, newStock.price, newStock.priceChange,
                             newStock.high52Week, newStock.low52Week, newStock.marketCap,
                             newStock.peRatio, newStock.dividendYield, newStock.eps,
                             newStock.volume, newStock.avgVolume, newStock.overview, newStock.analystRating);
    
    if (stock) {
        stockTree = insertStock(stockTree, stock);
        saveAllData();
        printf("Stock added successfully!\n");
        return true;
    }
    
    printf("Failed to add stock.\n");
    return false;
}

bool removeStock(const char* symbol) {

    Stock* stock = searchStock(stockTree, symbol);
    if (!stock) {
        printf("Stock not found!\n");
        return false;
    }
    printf("Stock %s removed from the system.\n", symbol);
    saveAllData();
    return true;
}

void updateStockPrice(Stock* stockTree) {
    char symbol[10];
    printf("Enter stock symbol to update: ");
    scanf("%9s", symbol);
    clearInputBuffer();
    
    Stock* stock = searchStock(stockTree, symbol);
    if (!stock) {
        printf("Stock not found!\n");
        return;
    }
    
    printf("Current price of %s is ₹%.2f\n", symbol, stock->price);
    printf("Enter new price: ");
    double newPrice;
    scanf("%lf", &newPrice);
    clearInputBuffer();

    float change = ((newPrice - stock->price) / stock->price) * 100;
    stock->priceChange = change;
    stock->price = newPrice;

    if (newPrice > stock->high52Week) {
        stock->high52Week = newPrice;
    }
    if (newPrice < stock->low52Week) {
        stock->low52Week = newPrice;
    }
    
    saveAllData();
    printf("Price updated successfully. New price: ₹%.2f (%.2f%% change)\n", 
          stock->price, stock->priceChange);
}

void showModifyStocksMenu() {
    int choice;
    do {
        printf("\n=== Stock Management ===\n");
        printf("1. Add New Stock\n");
        printf("2. Remove Stock\n");
        printf("3. Update Stock Price\n");
        printf("4. View All Stocks\n");
        printf("5. Back to Admin Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1:
                addStock();
                break;
            case 2: {
                char symbol[10];
                printf("Enter stock symbol to remove: ");
                scanf("%9s", symbol);
                clearInputBuffer();
                removeStock(symbol);
                break;
            }
            case 3:
                updateStockPrice(stockTree);
                break;
            case 4:
                displayStocks(stockTree);
                break;
            case 5:
                return;
            default:
                printf("Invalid choice. Try again.\n");
        }
    } while (choice != 5);
}

// ================== Menu Functions ==================

void showLoginMenu() {
    printf(GREEN);
printf(" _______             _           _  __  __        \n");
printf("|__   __|           | |         | |/ / / _|       \n");
printf("   | |_ __ __ _  ___| |__   __ _| ' / | |_ ___    \n");
printf("   | || '__/ _` |/ __| '_ \\ / _` |  <  |  _/ _ \\ \n");
printf("   | ||| | (_| | (__| | | | (_| | . \\ | || (_) |  \n");
printf("   |_||_|  \\__,_|\\___|_| |_|\\__,_|_|\\_\\|_| \\___/ \n");
printf("                     T R A D E X                \n");
printf(RESET);
    setCLIStyle();
    int choice;
    do {
        typewriter("\n=== TradeX - Stock Trading System ===\n", 10);
        typewriter("1. Login\n", 10);
        typewriter("2. Register\n", 10);
        typewriter("3. Forgot Password\n", 10);
        typewriter("4. Exit\n", 10);
        typewriter("Enter your choice: ", 10);
        
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1:
                if (loginUser()) {
                    if (strcmp(currentUser->role, "admin") == 0) {
                        adminMenu();
                    } else {
                        userMenu();
                    }
                }
                break;
            case 2:
                registerUser();
                break;
            case 3:
                forgotPassword();
                break;
            case 4:
                typewriter("Exiting system. Goodbye!\n", 10);
                exit(0);
            default:
                typewriter("Invalid choice. Try again.\n", 10);
        }
    } while (1);
}



void showAdminMenu() {
    printf("\n=== Admin Dashboard ===\n");
    printf("1. User Management\n");
    printf("2. Stock Management\n");
    printf("3. System Backup\n");
    printf("4. View All Users\n");
    printf("5. Reset User Password\n");
    printf("6. Logout\n");
    printf("Enter your choice: ");
}

void showUserMenu() {
    printf("\n=== User Dashboard ===\n");
    printf("1. View Stocks\n");
    printf("2. Buy Stocks\n");
    printf("3. Sell Stocks\n");
    printf("4. View Portfolio\n");
    printf("5. Account Balance\n");
    printf("6. Transaction History\n");
    printf("7. Deposit Money\n");
    printf("8. Withdraw Money\n");
    printf("9. Logout\n");
    printf("Enter your choice: ");
}

void adminMenu() {
    if (!currentUser || strcmp(currentUser->role, "admin") != 0) {
        typewriter("Admin access required!\n", 10);
        return;
    }
    
    if (!authenticateAdmin()) {
        return;
    }
    
    int choice;
    do {
        clearScreen();
        typewriter("\n=== Admin Dashboard ===\n", 10);
        typewriter("1. User Management\n", 10);
        typewriter("2. Stock Management\n", 10);
        typewriter("3. System Backup\n", 10);
        typewriter("4. View All Users\n", 10);
        typewriter("5. Reset User Password\n", 10);
        typewriter("6. Logout\n", 10);
        typewriter("Enter your choice: ", 10);
        
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1:
                typewriter("User management functionality would go here\n", 10);
                break;
            case 2:
                showModifyStocksMenu();
                break;
            case 3:
                createBackup();
                break;
            case 4:
                displayAllUsers();
                break;
            case 5:
                resetUserPassword();
                break;
            case 6:
                logoutUser();
                return;
            default:
                typewriter("Invalid choice. Try again.\n", 10);
        }
        sleep(1); 
    } while (1);
}

void userMenu() {
    if (!currentUser) {
        typewriter("No user logged in!\n", 10);
        return;
    }
    
    int choice;
    do {
        clearScreen();
        typewriter("\n=== User Dashboard ===\n", 10);
        typewriter("1. View Stocks\n", 10);
        typewriter("2. Buy Stocks\n", 10);
        typewriter("3. Sell Stocks\n", 10);
        typewriter("4. View Portfolio\n", 10);
        typewriter("5. Account Balance\n", 10);
        typewriter("6. Transaction History\n", 10);
        typewriter("7. Deposit Money\n", 10);
        typewriter("8. Withdraw Money\n", 10);
        typewriter("9. Logout\n", 10);
        typewriter("Enter your choice: ", 10);
        
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1: {
                typewriter("\n=== Available Stocks ===\n", 10);
                displayStocks(stockTree);
                break;
            }
            case 2: {
                BuyOrder();
                break;
            }
            case 3: {
                SellOrder();
                break;
            }
            case 4:
                displayPortfolio();
                break;
            case 5:
                displayBalance(currentUser);
                break;
            case 6:
                displayTransactionHistory(currentUser);
                break;
            case 7: {
                double amount;
                typewriter("Enter amount to deposit: ", 10);
                scanf("%lf", &amount);
                clearInputBuffer();
                creditUser(currentUser, amount, "Deposit");
                break;
            }
            case 8: {
                double amount;
                typewriter("Enter amount to withdraw: ", 10);
                scanf("%lf", &amount);
                clearInputBuffer();
                debitUser(currentUser, amount, "Withdrawal");
                break;
            }
            case 9:
                logoutUser();
                return;
            default:
                typewriter("Invalid choice. Try again.\n", 10);
        }
        typewriter("\nPress Enter to continue...", 10);
        while (getchar() != '\n'); 
    } while (1);
}

void displayAllUsers() {
    printf("\n=== All Registered Users ===\n");
    printf("%-20s %-30s %-10s %-15s\n", "Username", "Email", "Role", "Balance");
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        User* user = usersHashTable[i];
        while (user) {
            printf("%-20s %-30s %-10s ₹%-14.2f\n", 
                  user->username, 
                  user->email, 
                  user->role, 
                  user->balance);
            user = user->next;
        }
    }
}

void resetUserPassword() {
    char username[50];
    printf("Enter username to reset password: ");
    scanf("%49s", username);
    clearInputBuffer();
    
    unsigned int index = hash(username);
    User* user = usersHashTable[index];
    
    while (user) {
        if (strcmp(user->username, username) == 0) {
            char newPass[100], confirm[100];
            int match = 0;
            do {
                getHiddenPassword("New password: ", newPass, sizeof(newPass));
                getHiddenPassword("Confirm password: ", confirm, sizeof(confirm));
                
                match = strcmp(newPass, confirm);
                if (match != 0) {
                    printf("Passwords don't match! Please try again.\n");
                }
            } while (match != 0);
            
            unsigned long hashed = simpleHash(newPass);
            snprintf(user->password, sizeof(user->password), "%lu", hashed);
            user->failed_attempts = 0;
            printf("Password for %s reset successfully!\n", username);
            saveUsersToFile();
            return;
        }
        user = user->next;
    }
    
    printf("User %s not found.\n", username);
}

// ================== Main Function ==================

int main() {
    loadAllData();
    initializeSampleStocks();
    showLoginMenu();
    return 0;
}