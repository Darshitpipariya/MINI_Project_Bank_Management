#ifndef CUSTOMER_FUNCTIONS
#define CUSTOMER_FUNCTIONS
#include <sys/ipc.h>
#include <sys/sem.h>
#include "./COMMON.h"
struct Customer loggedInCustomer;
int semId;
bool customer_operation(int connFD);
bool deposit(int connFD, int acc_type);
bool withdraw(int connFD, int acc_type);
bool get_balance(int connFD, int acc_type);
bool change_password(int connFD);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);
bool customer_operation(int connFD)
{
    if (login(false, connFD, &loggedInCustomer))
    {
        ssize_t wBytes, rBytes;            // Number of bytes read from / written to the client
        char rBuffer[1000], wBuffer[1000]; // A buffer used for reading & writing to the client

        // Get a semaphore for the user
        key_t semKey = ftok(CUSTOMERS, loggedInCustomer.account); // Generate a key based on the account number hence, different customers will have different semaphores

        union semun
        {
            int val; // Value of the semaphore
        } semSet;

        int semctlStatus;
        semId = semget(semKey, 1, 0); // Get the semaphore if it exists
        if (semId == -1)
        {
            semId = semget(semKey, 1, IPC_CREAT | 0700); // Create a new semaphore
            if (semId == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            semSet.val = 1; // Set a binary semaphore
            semctlStatus = semctl(semId, 0, SETVAL, semSet);
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }

        wBytes = write(connFD, "Select account type\n1.Normal\n2.joint\n", strlen("Select account type\n1.Normal\n2.joint\n"));
        if (wBytes == -1)
        {
            perror("Error writing account type message to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, &rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading account type response from client!");
            return false;
        }
        int acc_type = atoi(rBuffer);


        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "Welcome beloved customer!");
        while (1)
        {
            strcat(wBuffer, "\n");
            strcat(wBuffer, "1. Get Customer Details\n2. Deposit Money\n3. Withdraw Money\n4. Get Balance\n5. Change Password\nPress any other key to logout");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing CUSTOMER_MENU to client!");
                return false;
            }
            bzero(wBuffer, sizeof(wBuffer));

            bzero(rBuffer, sizeof(rBuffer));
            rBytes = read(connFD, rBuffer, sizeof(rBuffer));
            if (rBytes == -1)
            {
                perror("Error while reading client's choice for CUSTOMER_MENU");
                return false;
            }

            
            int choice = atoi(rBuffer);
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, loggedInCustomer.id);
                break;
            case 2:
                deposit(connFD,acc_type);
                break;
            case 3:
                withdraw(connFD,acc_type);
                break;
            case 4:
                get_balance(connFD,acc_type);
                break;
            case 5:
                change_password(connFD);
                break;
            default:
                wBytes = write(connFD, "Logging you out now dear customer! Good bye!$", strlen("Logging you out now dear customer! Good bye!$"));
                return false;
            }
        }
    }
    else
    {
        // CUSTOMER LOGIN FAILED
        return false;
    }
    return true;
}

bool deposit(int connFD,int acc_type)
{
    char rBuffer[1000], wBuffer[1000];
    ssize_t rBytes, wBytes;

    if(acc_type==1){
        /***********************************************/
        /*NORMAL CUSTOMER*/
        /***********************************************/
        struct Normal_Account account;
        account.accountNumber = loggedInCustomer.account;

        long int depositAmount = 0;

        // Lock the critical section
        struct sembuf semOp;
        lock_critical_section(&semOp);

        if (get_Normal_account_details(connFD, &account))
        {

            if (account.active)
            {

                wBytes = write(connFD, "How much is it that you want to add into your bank?", strlen("How much is it that you want to add into your bank?"));
                if (wBytes == -1)
                {
                    perror("Error writing DEPOSIT_AMOUNT to client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                bzero(rBuffer, sizeof(rBuffer));
                rBytes = read(connFD, rBuffer, sizeof(rBuffer));
                if (rBytes == -1)
                {
                    perror("Error reading deposit money from client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                depositAmount = atol(rBuffer);
                if (depositAmount != 0)
                {
                    account.balance += depositAmount;

                    int accountFileDescriptor = open(NORMAL_ACCOUNTS, O_WRONLY);
                    off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Normal_Account), SEEK_SET);

                    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Normal_Account), getpid()};
                    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                    if (lockingStatus == -1)
                    {
                        perror("Error obtaining write lock on account file!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    wBytes = write(accountFileDescriptor, &account, sizeof(struct Normal_Account));
                    if (wBytes == -1)
                    {
                        perror("Error storing updated deposit money in account record!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    lock.l_type = F_UNLCK;
                    fcntl(accountFileDescriptor, F_SETLK, &lock);

                    write(connFD, "The specified amount has been successfully added to your bank account!^", strlen("The specified amount has been successfully added to your bank account!^"));
                    read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

                    get_balance(connFD,acc_type);

                    unlock_critical_section(&semOp);

                    return true;
                }
                else{
                    wBytes = write(connFD, "You seem to have passed an invalid amount!^", strlen("You seem to have passed an invalid amount!^"));
                }
            }
            else{
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            }
            read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            unlock_critical_section(&semOp);
        }
        else
        {
            // FAIL
            unlock_critical_section(&semOp);
            return false;
        }
    }else if (acc_type==2)
    {
        /***********************************************/
        /*JOINT CUSTOMER*/
        /***********************************************/
        struct Joint_Account account;
        account.accountNumber = loggedInCustomer.account;

        long int depositAmount = 0;

        // Lock the critical section
        struct sembuf semOp;
        lock_critical_section(&semOp);

        if (get_Joint_account_details(connFD, &account))
        {

            if (account.active)
            {

                wBytes = write(connFD, "How much is it that you want to add into your bank?", strlen("How much is it that you want to add into your bank?"));
                if (wBytes == -1)
                {
                    perror("Error writing DEPOSIT_AMOUNT to client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                bzero(rBuffer, sizeof(rBuffer));
                rBytes = read(connFD, rBuffer, sizeof(rBuffer));
                if (rBytes == -1)
                {
                    perror("Error reading deposit money from client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                depositAmount = atol(rBuffer);
                if (depositAmount != 0)
                {
                    account.balance += depositAmount;

                    int accountFileDescriptor = open(JOINT_ACCOUNTS, O_WRONLY);
                    off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Joint_Account), SEEK_SET);

                    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Joint_Account), getpid()};
                    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                    if (lockingStatus == -1)
                    {
                        perror("Error obtaining write lock on account file!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    wBytes = write(accountFileDescriptor, &account, sizeof(struct Joint_Account));
                    if (wBytes == -1)
                    {
                        perror("Error storing updated deposit money in account record!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    lock.l_type = F_UNLCK;
                    fcntl(accountFileDescriptor, F_SETLK, &lock);

                    write(connFD, "The specified amount has been successfully added to your bank account!^", strlen("The specified amount has been successfully added to your bank account!^"));
                    read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

                    get_balance(connFD,acc_type);

                    unlock_critical_section(&semOp);

                    return true;
                }
                else{
                    wBytes = write(connFD, "You seem to have passed an invalid amount!^", strlen("You seem to have passed an invalid amount!^"));
                }
            }
            else
            {
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            }
            read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            unlock_critical_section(&semOp);
        }
        else
        {
            // FAIL
            unlock_critical_section(&semOp);
            return false;
        }
    }else{
        return false;
    }
}

bool withdraw(int connFD, int acc_type)
{
    char rBuffer[1000], wBuffer[1000];
    ssize_t rBytes, wBytes;

    if(acc_type==1){
        /***********************************************/
        /*NORMAL CUSTOMER*/
        /***********************************************/
        struct Normal_Account account;
        account.accountNumber = loggedInCustomer.account;

        long int withdrawAmount = 0;

        // Lock the critical section
        struct sembuf semOp;
        lock_critical_section(&semOp);

        if (get_Normal_account_details(connFD, &account))
        {
            if (account.active)
            {

                wBytes = write(connFD, "How much is it that you want to withdraw from your bank?", strlen("How much is it that you want to withdraw from your bank?"));
                if (wBytes == -1)
                {
                    perror("Error writing WITHDRAW_AMOUNT message to client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                bzero(rBuffer, sizeof(rBuffer));
                rBytes = read(connFD, rBuffer, sizeof(rBuffer));
                if (rBytes == -1)
                {
                    perror("Error reading withdraw amount from client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                withdrawAmount = atol(rBuffer);

                if (withdrawAmount != 0 && account.balance - withdrawAmount >= 0)
                {
                    account.balance -= withdrawAmount;

                    int accountFileDescriptor = open(NORMAL_ACCOUNTS, O_WRONLY);
                    off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Normal_Account), SEEK_SET);

                    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Normal_Account), getpid()};
                    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                    if (lockingStatus == -1)
                    {
                        perror("Error obtaining write lock on account record!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    wBytes = write(accountFileDescriptor, &account, sizeof(struct Normal_Account));
                    if (wBytes == -1)
                    {
                        perror("Error writing updated balance into account file!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    lock.l_type = F_UNLCK;
                    fcntl(accountFileDescriptor, F_SETLK, &lock);

                    write(connFD, "The specified amount has been successfully withdrawn from your bank account!^", strlen("The specified amount has been successfully withdrawn from your bank account!^"));
                    read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

                    get_balance(connFD,acc_type);

                    unlock_critical_section(&semOp);

                    return true;
                }
                else{
                    wBytes = write(connFD, "You seem to have either passed an invalid amount or you don't have enough money in your bank to withdraw the specified amount^", strlen("You seem to have either passed an invalid amount or you don't have enough money in your bank to withdraw the specified amount^"));
                }
            }
            else{
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            }
            read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            unlock_critical_section(&semOp);
        }
        else
        {
            // FAILURE while getting account information
            unlock_critical_section(&semOp);
            return false;
        }
    }else if(acc_type==2){
        /***********************************************/
        /*JOINT CUSTOMER*/
        /***********************************************/
        struct Joint_Account account;
        account.accountNumber = loggedInCustomer.account;

        long int withdrawAmount = 0;

        // Lock the critical section
        struct sembuf semOp;
        lock_critical_section(&semOp);

        if (get_Joint_account_details(connFD, &account))
        {
            if (account.active)
            {

                wBytes = write(connFD, "How much is it that you want to withdraw from your bank?", strlen("How much is it that you want to withdraw from your bank?"));
                if (wBytes == -1)
                {
                    perror("Error writing WITHDRAW_AMOUNT message to client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                bzero(rBuffer, sizeof(rBuffer));
                rBytes = read(connFD, rBuffer, sizeof(rBuffer));
                if (rBytes == -1)
                {
                    perror("Error reading withdraw amount from client!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                withdrawAmount = atol(rBuffer);

                if (withdrawAmount != 0 && account.balance - withdrawAmount >= 0)
                {
                    account.balance -= withdrawAmount;

                    int accountFileDescriptor = open(JOINT_ACCOUNTS, O_WRONLY);
                    off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Joint_Account), SEEK_SET);

                    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Joint_Account), getpid()};
                    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                    if (lockingStatus == -1)
                    {
                        perror("Error obtaining write lock on account record!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    wBytes = write(accountFileDescriptor, &account, sizeof(struct Joint_Account));
                    if (wBytes == -1)
                    {
                        perror("Error writing updated balance into account file!");
                        unlock_critical_section(&semOp);
                        return false;
                    }

                    lock.l_type = F_UNLCK;
                    fcntl(accountFileDescriptor, F_SETLK, &lock);

                    write(connFD, "The specified amount has been successfully withdrawn from your bank account!^", strlen("The specified amount has been successfully withdrawn from your bank account!^"));
                    read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

                    get_balance(connFD,acc_type);

                    unlock_critical_section(&semOp);

                    return true;
                }
                else{
                    wBytes = write(connFD, "You seem to have either passed an invalid amount or you don't have enough money in your bank to withdraw the specified amount^", strlen("You seem to have either passed an invalid amount or you don't have enough money in your bank to withdraw the specified amount^"));
                }
            }
            else
            {
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            }
            read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            unlock_critical_section(&semOp);
        }
        else
        {
            // FAILURE while getting account information
            unlock_critical_section(&semOp);
            return false;
        }
    }
    else{
        return false;
    }
}

bool get_balance(int connFD, int acc_type)
{
    char buffer[1000];
    if(acc_type==1){
        /***********************************************/
        /*NORMAL CUSTOMER*/
        /***********************************************/
        struct Normal_Account account;
        account.accountNumber = loggedInCustomer.account;
        if (get_Normal_account_details(connFD, &account))
        {
            bzero(buffer, sizeof(buffer));
            if (account.active)
            {
                sprintf(buffer, "You have ₹ %ld imaginary money in our bank!^", account.balance);
                write(connFD, buffer, strlen(buffer));
            }
            else
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            read(connFD, buffer, sizeof(buffer)); // Dummy read
        }
        else
        {
            // ERROR while getting balance
            return false;
        }
    }else if(acc_type=2){
        /***********************************************/
        /*JOINT CUSTOMER*/
        /***********************************************/
        struct Joint_Account account;
        account.accountNumber = loggedInCustomer.account;
        if (get_Joint_account_details(connFD, &account))
        {
            bzero(buffer, sizeof(buffer));
            if (account.active)
            {
                sprintf(buffer, "You have ₹ %ld imaginary money in our bank!^", account.balance);
                write(connFD, buffer, strlen(buffer));
            }
            else
                write(connFD, "It seems your account has been deactivated!^", strlen("It seems your account has been deactivated!^"));
            read(connFD, buffer, sizeof(buffer)); // Dummy read
        }
        else
        {
            // ERROR while getting balance
            return false;
        }
    }else{
        return false;
    }
    
}

bool change_password(int connFD)
{
    ssize_t rBytes, wBytes;
    char rBuffer[1000], wBuffer[1000], hashedPassword[1000];

    char newPassword[1000];

    // Lock the critical section
    struct sembuf semOp = {0, -1, SEM_UNDO};
    int semopStatus = semop(semId, &semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }

    wBytes = write(connFD, "Enter your old password", strlen("Enter your old password"));
    if (wBytes == -1)
    {
        perror("Error writing PASSWORD_CHANGE_OLD_PASS message to client!");
        unlock_critical_section(&semOp);
        return false;
    }

    bzero(rBuffer, sizeof(rBuffer));
    rBytes = read(connFD, rBuffer, sizeof(rBuffer));
    if (rBytes == -1)
    {
        perror("Error reading old password response from client");
        unlock_critical_section(&semOp);
        return false;
    }

    if (strcmp(rBuffer, loggedInCustomer.password) == 0)
    {
        // Password matches with old password
        wBytes = write(connFD, "Enter the new password", strlen("Enter the new password"));
        if (wBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading new password response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        strcpy(newPassword, rBuffer);

        wBytes = write(connFD, "Reenter the new password", strlen("Reenter the new password"));
        if (wBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS_RE message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading new password reenter response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        if (strcmp(rBuffer, newPassword) == 0)
        {
            // New & reentered passwords match

            strcpy(loggedInCustomer.password, newPassword);

            int customerFileDescriptor = open(CUSTOMERS, O_WRONLY);
            if (customerFileDescriptor == -1)
            {
                perror("Error opening customer file!");
                unlock_critical_section(&semOp);
                return false;
            }

            off_t offset = lseek(customerFileDescriptor, loggedInCustomer.id * sizeof(struct Customer), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
            int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            wBytes = write(customerFileDescriptor, &loggedInCustomer, sizeof(struct Customer));
            if (wBytes == -1)
            {
                perror("Error storing updated customer password into customer record!");
                unlock_critical_section(&semOp);
                return false;
            }

            lock.l_type = F_UNLCK;
            lockingStatus = fcntl(customerFileDescriptor, F_SETLK, &lock);

            close(customerFileDescriptor);

            wBytes = write(connFD, "Password successfully changed!^", strlen("Password successfully changed!^"));
            rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

            unlock_critical_section(&semOp);

            return true;
        }
        else
        {
            // New & reentered passwords don't match
            wBytes = write(connFD, "The new password and the reentered passwords don't seem to pass!^", strlen("The new password and the reentered passwords don't seem to pass!^"));
            rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        }
    }
    else
    {
        // Password doesn't match with old password
        wBytes = write(connFD, "The entered password doesn't seem to match with the old password", strlen("The entered password doesn't seem to match with the old password"));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
    }

    unlock_critical_section(&semOp);

    return false;
}

bool lock_critical_section(struct sembuf *semOp)
{
    semOp->sem_flg = SEM_UNDO;
    semOp->sem_op = -1;
    semOp->sem_num = 0;
    int semopStatus = semop(semId, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }
    return true;
}

bool unlock_critical_section(struct sembuf *semOp)
{
    semOp->sem_op = 1;
    int semopStatus = semop(semId, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}

#endif