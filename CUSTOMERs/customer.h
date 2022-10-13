#ifndef CUSTOMER_RECORD
#define CUSTOMER_RECORD

struct Customer
{
    int id; // 0, 1, 2 ....
    char name[25];
    // Login Credentials
    char login[30]; // Format : name-id (name will the first word in the structure member `name`)
    char password[30];
    // Bank data
    int account; // Account number of the account the customer owns
};

#endif