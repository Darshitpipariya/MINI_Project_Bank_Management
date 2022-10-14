#ifndef COMMON_FUNCTIONS
#define COMMON_FUNCTIONS

#include <stdio.h>     // Import for `printf` & `perror`
#include <unistd.h>    // Import for `read`, `write & `lseek`
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include "./ACCOUNTS/JOINT/Joint_Struct.h"
#include "./ACCOUNTS/NORMAL/Normal_Struct.h"
#include "./CUSTOMERs/customer.h"
#define NORMAL_ACCOUNTS "./ACCOUNTS/NORMAL/Normalaccount.txt"
#define JOINT_ACCOUNTS "./ACCOUNTS/JOINT/Jointaccount.txt"
#define CUSTOMERS "./CUSTOMERs/customer.txt"
#define ADMIN_LOGIN_ID "admin"
#define ADMIN_PASSWORD "admin"
#define AUTOGEN_PASSWORD "admin"

bool login(bool isAdmin, int connFD, struct Customer *ptrToCustomer);
bool get_Normal_account_details(int connFD, struct Normal_Account *customerAccount);
bool get_Joint_account_details(int connFD, struct Joint_Account *customerAccount);
bool get_customer_details(int connFD, int customerID);

bool login(bool isAdmin, int connFD, struct Customer *ptrToCustomerID)
{
    ssize_t rBytes, wBytes;            // Number of bytes written to / read from the socket
    char rBuffer[1000], wBuffer[1000]; // Buffer for reading from / writing to the client
    char tBuffer[1000];
    struct Customer customer;

    int ID;

    bzero(rBuffer, sizeof(rBuffer));
    bzero(wBuffer, sizeof(wBuffer));

    // Get login message for respective user type
    if (isAdmin)
        strcpy(wBuffer, "Welcome dear admin\nEnter your credentials ");
    else
        strcpy(wBuffer, "Welcome dear customer! Enter your credentials ");

    // Append the request for LOGIN ID message
    strcat(wBuffer, "\n");
    strcat(wBuffer, "Enter your login ID");

    wBytes = write(connFD, wBuffer, strlen(wBuffer));
    if (wBytes == -1)
    {
        perror("Error writing WELCOME & LOGIN_ID message to the client!");
        return false;
    }

    rBytes = read(connFD, rBuffer, sizeof(rBuffer));
    if (rBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }

    bool userFound = false;

    if (isAdmin)
    {
        if (strcmp(rBuffer, ADMIN_LOGIN_ID) == 0)
            userFound = true;
    }
    else
    {
        bzero(tBuffer, sizeof(tBuffer));
        strcpy(tBuffer, rBuffer);
        strtok(tBuffer, "-");
        ID = atoi(strtok(NULL, "-"));
        printf("%d\n",ID);
        int customerFileFD = open(CUSTOMERS, O_RDONLY);
        if (customerFileFD == -1)
        {
            perror("Error opening customer file in read mode!");
            return false;
        }

        off_t offset = lseek(customerFileFD, ID * sizeof(struct Customer), SEEK_SET);
        if (offset >= 0)
        {
            struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Customer), sizeof(struct Customer), getpid()};

            int lockingStatus = fcntl(customerFileFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on customer record!");
                return false;
            }

            rBytes = read(customerFileFD, &customer, sizeof(struct Customer));
            if (rBytes == -1)
            {
                ;
                perror("Error reading customer record from file!");
            }

            lock.l_type = F_UNLCK;
            fcntl(customerFileFD, F_SETLK, &lock);

            if (strcmp(customer.login, rBuffer) == 0)
                userFound = true;

            close(customerFileFD);
        }
        else
        {
            wBytes = write(connFD, "No customer could be found for the given login ID$", strlen("No customer could be found for the given login ID$"));
        }
    }

    if (userFound)
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD, "Enter your password \n#", strlen("Enter your password \n#"));
        if (wBytes == -1)
        {
            perror("Error writing PASSWORD message to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == 1)
        {
            perror("Error reading password from the client!");
            return false;
        }

        if (isAdmin)
        {
            if (strcmp(rBuffer, ADMIN_PASSWORD) == 0)
                return true;
        }
        else
        {
            if (strcmp(rBuffer, customer.password) == 0)
            {
                *ptrToCustomerID = customer;
                return true;
            }
        }

        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD, "The password specified doesn't match!$", strlen("The password specified doesn't match!$"));
    }
    else
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD, "The login ID specified doesn't exist!$", strlen("The login ID specified doesn't exist!$"));
    }

    return false;
}

bool get_Normal_account_details(int connFD, struct Normal_Account *customerAccount)
{
    ssize_t rBytes, wBytes;            // Number of bytes read from / written to the socket
    char rBuffer[1000], wBuffer[1000]; // A buffer for reading from / writing to the socket
    char tBuffer[1000];

    int accountNumber;
    /***********************************************/
    /*NORMAL CUSTOMER*/
    /***********************************************/
    struct Normal_Account account;
    int accountFileDescriptor;

    if (customerAccount == NULL)
    {

        wBytes = write(connFD, "Enter the account number of the account you're searching for", strlen("Enter the account number of the account you're searching for"));
        if (wBytes == -1)
        {
            perror("Error writing GET_ACCOUNT_NUMBER message to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading account number response from client!");
            return false;
        }

        accountNumber = atoi(rBuffer);
    }
    else
    {
        accountNumber = customerAccount->accountNumber;
    }
    accountFileDescriptor = open(NORMAL_ACCOUNTS, O_RDONLY);
    if (accountFileDescriptor == -1)
    {
        // Account record doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "No account could be found for the given account number");
        strcat(wBuffer, "^");
        perror("Error opening account file in get_account_details!");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }

    int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Normal_Account), SEEK_SET);
    if (offset == -1 && errno == EINVAL)
    {
        // Account record doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "No account could be found for the given account number");
        strcat(wBuffer, "^");
        perror("Error seeking to account record in get_account_details!");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required account record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Normal_Account), getpid()};

    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on account record!");
        return false;
    }

    rBytes = read(accountFileDescriptor, &account, sizeof(struct Normal_Account));
    if (rBytes == -1)
    {
        perror("Error reading account record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    if (customerAccount != NULL)
    {
        *customerAccount = account;
        return true;
    }

    bzero(wBuffer, sizeof(wBuffer));
    sprintf(wBuffer, "Account Details - \n\tAccount Number : %d\n\tAccount Type : Normal\n\tAccount Status : %s", account.accountNumber, (account.active) ? "Active" : "Deactived");
    if (account.active)
    {
        sprintf(tBuffer, "\n\tAccount Balance:₹ %ld", account.balance);
        strcat(wBuffer, tBuffer);
    }

    sprintf(tBuffer, "\n\t Owner ID: %d", account.owner);
    strcat(wBuffer, tBuffer);
    strcat(wBuffer, "\n^");

    wBytes = write(connFD, wBuffer, strlen(wBuffer));
    rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

    return true;
}
bool get_Joint_account_details(int connFD, struct Joint_Account *customerAccount)
{
    ssize_t rBytes, wBytes;            // Number of bytes read from / written to the socket
    char rBuffer[1000], wBuffer[1000]; // A buffer for reading from / writing to the socket
    char tBuffer[1000];

    int accountNumber;
    /***********************************************/
    /*JOINT CUSTOMER*/
    /***********************************************/
    struct Joint_Account account;
    int accountFileDescriptor;

    if (customerAccount == NULL)
    {

        wBytes = write(connFD, "Enter the account number of the account you're searching for", strlen("Enter the account number of the account you're searching for"));
        if (wBytes == -1)
        {
            perror("Error writing GET_ACCOUNT_NUMBER message to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading account number response from client!");
            return false;
        }

        accountNumber = atoi(rBuffer);
    }
    else
        accountNumber = customerAccount->accountNumber;

    accountFileDescriptor = open(JOINT_ACCOUNTS, O_RDONLY);
    if (accountFileDescriptor == -1)
    {
        // Account record doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "Enter the account number of the account you're searching for");
        strcat(wBuffer, "^");
        perror("Error opening account file in get_account_details!");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }

    int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Joint_Account), SEEK_SET);
    if (offset == -1 && errno == EINVAL)
    {
        // Account record doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "Enter the account number of the account you're searching for");
        strcat(wBuffer, "^");
        perror("Error seeking to account record in get_account_details!");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required account record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Joint_Account), getpid()};

    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on account record!");
        return false;
    }

    rBytes = read(accountFileDescriptor, &account, sizeof(struct Joint_Account));
    if (rBytes == -1)
    {
        perror("Error reading account record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    if (customerAccount != NULL)
    {
        *customerAccount = account;
        return true;
    }

    bzero(wBuffer, sizeof(wBuffer));
    sprintf(wBuffer, "Account Details - \n\tAccount Number : %d\n\tAccount Type : Joint\n\tAccount Status : %s", account.accountNumber, (account.active) ? "Active" : "Deactived");
    if (account.active)
    {
        sprintf(tBuffer, "\n\tAccount Balance:₹ %ld", account.balance);
        strcat(wBuffer, tBuffer);
    }

    sprintf(tBuffer, "\n\t Owner_1 ID: %d", account.owner_1);
    strcat(wBuffer, tBuffer);
    sprintf(tBuffer, "\n\t Owner_2 ID: %d", account.owner_2);
    strcat(wBuffer, tBuffer);
    strcat(wBuffer, "\n^");

    wBytes = write(connFD, wBuffer, strlen(wBuffer));
    rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

    return true;
}

bool get_customer_details(int connFD, int customerID)
{
    ssize_t rBytes, wBytes;             // Number of bytes read from / written to the socket
    char rBuffer[1000], wBuffer[10000]; // A buffer for reading from / writing to the socket
    char tBuffer[1000];

    struct Customer customer;
    int customerFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Customer), getpid()};

    if (customerID == -1)
    {
        wBytes = write(connFD, "Enter the customer ID of the customer you're searching for", strlen("Enter the customer ID of the customer you're searching for"));
        if (wBytes == -1)
        {
            perror("Error while writing GET_CUSTOMER_ID message to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error getting customer ID from client!");
            ;
            return false;
        }

        customerID = atoi(rBuffer);
    }

    customerFileDescriptor = open(CUSTOMERS, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "No customer could be found for the given ID");
        strcat(wBuffer, "^");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }
    int offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "No customer could be found for the given ID");
        strcat(wBuffer, "^");
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }
    lock.l_start = offset;

    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Customer file!");
        return false;
    }

    rBytes = read(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (rBytes == -1)
    {
        perror("Error reading customer record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);

    bzero(wBuffer, sizeof(wBuffer));
    sprintf(wBuffer, "Customer Details - \n\tID : %d\n\tName : %s\n\tAccount Number : %d\n\tLoginID : %s", customer.id, customer.name, customer.account, customer.login);

    strcat(wBuffer, "\n\nYou'll now be redirected to the main menu...^");

    wBytes = write(connFD, wBuffer, strlen(wBuffer));
    if (wBytes == -1)
    {
        perror("Error writing customer info to client!");
        return false;
    }

    rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
    return true;
}
// =====================================================

#endif