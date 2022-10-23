#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCTIONS


#include "./COMMON.h"


int add_customer(int connFD, int newAccountNumber);
bool add_account(int connFD);
bool delete_account(int connFD);
bool admin_operation(int connFD);

bool admin_operation(int connFD)
{

    if (login(true, connFD, NULL))
    {
        ssize_t wBytes, rBytes;            // Number of bytes read from / written to the client
        char rBuffer[1000], wBuffer[1000]; // A buffer used for reading & writing to the client
        bzero(wBuffer, sizeof(wBuffer));
        strcpy(wBuffer, "Welcome admin!");
        while (1)
        {
            strcat(wBuffer, "\n");
            strcat(wBuffer, "1. Get Customer Details\n2. Get Normal Account Details\n3. Get Joint Account Details\n4. Add Account\n5. Delete Account\n6. Get Normal Account Transaction Details\n7. Get Joint Account Transcation Details\nPress any other key to logout");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(wBuffer, sizeof(wBuffer));

            rBytes = read(connFD, rBuffer, sizeof(rBuffer));
            if (rBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(rBuffer);
            switch (choice)
            {
            case 1:
                get_customer_details(connFD, -1);
                break;
            case 2:
                get_Normal_account_details(connFD, NULL);
                break;
            case 3:
                get_Joint_account_details(connFD, NULL);
                break;
            case 4:
                add_account(connFD);
                break;
            case 5:
                delete_account(connFD);
                break;
            case 6:
                get_transaction_details(connFD,-1,1);
                break;
            case 7:
                get_transaction_details(connFD,-1,2);
                break;
            default:
                wBytes = write(connFD, "Logging you out now superman! Good bye!$", strlen("Logging you out now superman! Good bye!$"));
                return false;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        return false;
    }
    return true;
}

bool add_account(int connFD)
{
    ssize_t rBytes, wBytes;
    char rBuffer[1000], wBuffer[1000];
    wBytes = write(connFD, "Select account type\n1.Normal\n2.joint\n", strlen("Select account type\n1.Normal\n2.joint\n"));
    if (wBytes == -1)
    {
        perror("Error writing admin account type message to client!");
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
    if (acc_type == 1)
    {
        /***********************************************/
        /*NORMAL CUSTOMER*/
        /***********************************************/
        struct Normal_Account newAccount, prevAccount;
        int accountFileDescriptor = open(NORMAL_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1 && errno == ENOENT)
        {
            // Account file was never created
            newAccount.accountNumber = 0;
        }
        else if (accountFileDescriptor == -1)
        {
            perror("Error while opening Normal account file");
            return false;
        }
        else
        {
            int offset = lseek(accountFileDescriptor, -sizeof(struct Normal_Account), SEEK_END);
            if (offset == -1)
            {
                perror("Error seeking to last Normal Account record!");
                return false;
            }

            struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Normal_Account), getpid()};
            int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on Normal Account record!");
                return false;
            }

            rBytes = read(accountFileDescriptor, &prevAccount, sizeof(struct Normal_Account));
            if (rBytes == -1)
            {
                perror("Error while reading Normal Account record from file!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            close(accountFileDescriptor);

            newAccount.accountNumber = prevAccount.accountNumber + 1;
        }
        // add owner name
        newAccount.owner = add_customer(connFD, newAccount.accountNumber);
        // set active
        newAccount.active = true;
        // set balance
        newAccount.balance = 0;
        memset(newAccount.transactions, -1, 5 * sizeof(int));

        accountFileDescriptor = open(NORMAL_ACCOUNTS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        if (accountFileDescriptor == -1)
        {
            perror("Error while creating opening Normal account file!");
            return false;
        }
        wBytes = write(accountFileDescriptor, &newAccount, sizeof(struct Normal_Account));
        if (wBytes == -1)
        {
            perror("Error while writing Normal Account record to file!");
            return false;
        }

        close(accountFileDescriptor);
        bzero(wBuffer, sizeof(wBuffer));
        sprintf(wBuffer, "The newly created account's number is :%d", newAccount.accountNumber);
        strcat(wBuffer, "\nRedirecting you to the main menu ...^");
        wBytes = write(connFD, wBuffer, sizeof(wBuffer));
        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return true;
    }
    else if (acc_type == 2)
    {
        /***********************************************/
        /*Joint CUSTOMER*/
        /***********************************************/
        struct Joint_Account newAccount, prevAccount;
        int accountFileDescriptor = open(JOINT_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1 && errno == ENOENT)
        {
            // Account file was never created
            newAccount.accountNumber = 0;
        }
        else if (accountFileDescriptor == -1)
        {
            perror("Error while opening Joint account file");
            return false;
        }
        else
        {
            int offset = lseek(accountFileDescriptor, -sizeof(struct Joint_Account), SEEK_END);
            if (offset == -1)
            {
                perror("Error seeking to last Joint Account record!");
                return false;
            }

            struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Joint_Account), getpid()};
            int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on Joint Account record!");
                return false;
            }

            rBytes = read(accountFileDescriptor, &prevAccount, sizeof(struct Joint_Account));
            if (rBytes == -1)
            {
                perror("Error while reading Joint Account record from file!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            close(accountFileDescriptor);

            newAccount.accountNumber = prevAccount.accountNumber + 1;
        }
        // add owner_1 name
        newAccount.owner_1 = add_customer(connFD, newAccount.accountNumber);
        // add owner_2 name
        newAccount.owner_2 = add_customer(connFD, newAccount.accountNumber);
        // set active
        newAccount.active = true;
        // set balance
        newAccount.balance = 0;
        memset(newAccount.transactions, -1, 5 * sizeof(int));
        accountFileDescriptor = open(JOINT_ACCOUNTS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        if (accountFileDescriptor == -1)
        {
            perror("Error while creating opening Joint account file!");
            return false;
        }
        wBytes = write(accountFileDescriptor, &newAccount, sizeof(struct Joint_Account));
        if (wBytes == -1)
        {
            perror("Error while writing Joint Account record to file!");
            return false;
        }

        close(accountFileDescriptor);
        bzero(wBuffer, sizeof(wBuffer));
        sprintf(wBuffer, "The newly created account's number is :%d", newAccount.accountNumber);
        strcat(wBuffer, "\nRedirecting you to the main menu ...^");
        wBytes = write(connFD, wBuffer, sizeof(wBuffer));
        bzero(rBuffer, sizeof(wBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return true;
    }
    else
    {
        return false;
    }
}

int add_customer(int connFD, int newAccountNumber)
{
    ssize_t rBytes, wBytes;
    char rBuffer[1000], wBuffer[1000];

    struct Customer newCustomer, previousCustomer;

    int customerFileDescriptor = open(CUSTOMERS, O_RDONLY);
    if (customerFileDescriptor == -1 && errno == ENOENT)
    {
        // Customer file was never created
        newCustomer.id = 0;
    }
    else if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return -1;
    }
    else
    {
        int offset = lseek(customerFileDescriptor, -sizeof(struct Customer), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Customer record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
        int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Customer record!");
            return false;
        }

        rBytes = read(customerFileDescriptor, &previousCustomer, sizeof(struct Customer));
        if (rBytes == -1)
        {
            perror("Error while reading Customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFileDescriptor, F_SETLK, &lock);

        close(customerFileDescriptor);

        newCustomer.id = previousCustomer.id + 1;
    }
    bzero(wBuffer, sizeof(wBuffer));
    sprintf(wBuffer, "Enter the details for the customer\nWhat is the customer's name?");

    wBytes = write(connFD, wBuffer, sizeof(wBuffer));
    if (wBytes == -1)
    {
        perror("Error writing add customer message to client!");
        return false;
    }
    bzero(rBuffer, sizeof(rBuffer));
    rBytes = read(connFD, rBuffer, sizeof(rBuffer));
    if (rBytes == -1)
    {
        perror("Error reading customer name response from client!");
        return false;
    }

    strcpy(newCustomer.name, rBuffer);
    // set account number
    newCustomer.account = newAccountNumber;

    strcpy(newCustomer.login, newCustomer.name);
    strcat(newCustomer.login, "-");
    sprintf(wBuffer, "%d", newCustomer.id);
    strcat(newCustomer.login, wBuffer);
    
    printf("\n%d\n", newCustomer.id);
    strcpy(newCustomer.password, AUTOGEN_PASSWORD);
    customerFileDescriptor = open(CUSTOMERS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (customerFileDescriptor == -1)
    {
        perror("Error while creating opening customer file!");
        return false;
    }
    wBytes = write(customerFileDescriptor, &newCustomer, sizeof(newCustomer));
    if (wBytes == -1)
    {
        perror("Error while writing Customer record to file!");
        return false;
    }

    close(customerFileDescriptor);

    bzero(wBuffer, sizeof(wBuffer));
    sprintf(wBuffer, "The autogenerated login ID for the customer is : %s-%d\nThe autogenerated password for the customer is : %s", newCustomer.name, newCustomer.id, newCustomer.password);
    strcat(wBuffer, "^");
    wBytes = write(connFD, wBuffer, strlen(wBuffer));
    if (wBytes == -1)
    {
        perror("Error sending customer loginID and password to the client!");
        return false;
    }
    bzero(rBuffer, sizeof(rBuffer));
    rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read

    return newCustomer.id;
}

bool delete_account(int connFD)
{
    ssize_t rBytes, wBytes;
    char rBuffer[1000], wBuffer[1000];

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


    if(acc_type==1){
        /***********************************************/
        /*NORMAL ACCOUNT*/
        /***********************************************/
        struct Normal_Account account;
        wBytes = write(connFD, "What is the account number of the account you want to delete?", strlen("What is the account number of the account you want to delete?"));
        if (wBytes == -1)
        {
            perror("Error writing admin delete account number to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading account number response from the client!");
            return false;
        }

        int accountNumber = atoi(rBuffer);

        int accountFileDescriptor = open(NORMAL_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1)
        {
            // Account record doesn't exist
            bzero(wBuffer, sizeof(wBuffer));
            strcpy(wBuffer,"No account could be found for the given account number" );
            strcat(wBuffer, "^");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing account id doesn't exist message to client!");
                return false;
            }
            rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            return false;
        }

        int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Normal_Account), SEEK_SET);
        if (errno == EINVAL)
        {
            // Customer record doesn't exist
            bzero(wBuffer, sizeof(wBuffer));
            strcpy(wBuffer, "No account could be found for the given account number");
            strcat(wBuffer, "^");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing account id doesn't exist message to client!");
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
            perror("Error obtaining read lock on Account record!");
            return false;
        }

        rBytes = read(accountFileDescriptor, &account, sizeof(struct Normal_Account));
        if (rBytes == -1)
        {
            perror("Error while reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        close(accountFileDescriptor);

        bzero(wBuffer, sizeof(wBuffer));
        if (account.balance == 0)
        {
            // No money, hence can close account
            account.active = false;
            accountFileDescriptor = open(NORMAL_ACCOUNTS, O_WRONLY);
            if (accountFileDescriptor == -1)
            {
                perror("Error opening Account file in write mode!");
                return false;
            }

            offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Normal_Account), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the Account!");
                return false;
            }

            lock.l_type = F_WRLCK;
            lock.l_start = offset;

            int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the Account file!");
                return false;
            }

            wBytes = write(accountFileDescriptor, &account, sizeof(struct Normal_Account));
            if (wBytes == -1)
            {
                perror("Error deleting account record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            strcpy(wBuffer, "This account has been successfully deleted\nRedirecting you to the main menu ...^");
        }
        else{
            // Account has some money ask customer to withdraw it
            strcpy(wBuffer, "This account cannot be deleted since it still has some money\nRedirecting you to the main menu ...^");
        }
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing final DEL message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return true;
    }
    else if (acc_type==2){
        /***********************************************/
        /*Joint ACCOUNT*/
        /***********************************************/
        struct Joint_Account account;
        wBytes = write(1, "What is the account number of the account you want to delete?", strlen("What is the account number of the account you want to delete?"));
        if (wBytes == -1)
        {
            perror("Error writing ADMIN_DEL_ACCOUNT_NO to client!");
            return false;
        }

        bzero(rBuffer, sizeof(rBuffer));
        rBytes = read(connFD, rBuffer, sizeof(rBuffer));
        if (rBytes == -1)
        {
            perror("Error reading account number response from the client!");
            return false;
        }

        int accountNumber = atoi(rBuffer);

        int accountFileDescriptor = open(JOINT_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1)
        {
            // Account record doesn't exist
            bzero(wBuffer, sizeof(wBuffer));
            strcpy(wBuffer, "No account could be found for the given account number");
            strcat(wBuffer, "^");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing account id doesn't exist message to client!");
                return false;
            }
            rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
            return false;
        }

        int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Joint_Account), SEEK_SET);
        if (errno == EINVAL)
        {
            // Customer record doesn't exist
            bzero(wBuffer, sizeof(wBuffer));
            strcpy(wBuffer, "No account could be found for the given account number");
            strcat(wBuffer, "^");
            wBytes = write(connFD, wBuffer, strlen(wBuffer));
            if (wBytes == -1)
            {
                perror("Error while writing account id doesn't exist message to client!");
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
            perror("Error obtaining read lock on Account record!");
            return false;
        }

        rBytes = read(accountFileDescriptor, &account, sizeof(struct Joint_Account));
        if (rBytes == -1)
        {
            perror("Error while reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        close(accountFileDescriptor);

        bzero(wBuffer, sizeof(wBuffer));
        if (account.balance == 0)
        {
            // No money, hence can close account
            account.active = false;
            accountFileDescriptor = open(JOINT_ACCOUNTS, O_WRONLY);
            if (accountFileDescriptor == -1)
            {
                perror("Error opening Account file in write mode!");
                return false;
            }

            offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Joint_Account), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the Account!");
                return false;
            }

            lock.l_type = F_WRLCK;
            lock.l_start = offset;

            int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the Account file!");
                return false;
            }

            wBytes = write(accountFileDescriptor, &account, sizeof(struct Joint_Account));
            if (wBytes == -1)
            {
                perror("Error deleting account record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            strcpy(wBuffer, "This account has been successfully deleted\nRedirecting you to the main menu ...^");
        }
        else
        {
            // Account has some money ask customer to withdraw it
            strcpy(wBuffer, "This account cannot be deleted since it still has some money\nRedirecting you to the main menu ...^");
        }
        wBytes = write(connFD, wBuffer, strlen(wBuffer));
        if (wBytes == -1)
        {
            perror("Error while writing final DEL message to client!");
            return false;
        }
        rBytes = read(connFD, rBuffer, sizeof(rBuffer)); // Dummy read
        return true;
    }
    else{
        return false;
    }
    
}

#endif