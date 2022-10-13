#ifndef NORAML_ACCOUNT_RECORD
#define NORMAL_ACCOUNT_RECORD

#define MAX_TRANSACTIONS 10

struct Normal_Account
{
    int accountNumber;                  // 0, 1, 2, ....
    int owner;                      // Customer ID
    bool active;                        // 1 -> Active, 0 -> Deactivated (Deleted)
    long int balance;                   // Amount of money in the account
    
};

#endif