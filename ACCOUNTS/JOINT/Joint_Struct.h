#ifndef JOINT_ACCOUNT_RECORD
#define JOINT_ACCOUNT_RECORD


struct Joint_Account
{
    int accountNumber; // 0, 1, 2, ....
    int owner_1;        // Customer 1 ID
    int owner_2;       // Customer 2 ID
    bool active;       // 1 -> Active, 0 -> Deactivated (Deleted)
    long int balance;  // Amount of money in the account
};

#endif