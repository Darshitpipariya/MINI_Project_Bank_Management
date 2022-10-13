#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCTIONS


#include "./COMMON.h"


int add_customer(int connFD, int newAccountNumber);
bool add_account(int connFD);
bool delete_account(int connFD);
bool admin_operation_handler(int connFD);

bool admin_operation_handler(int connFD)
{

    if (login_handler(true, connFD, NULL))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Welcome admin!");
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, "1. Get Customer Details\n2. Get Normal Account Details\n3. Get Joint Account Details\n4. Add Account\n5. Delete Account\nPress any other key to logout");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
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
            default:
                writeBytes = write(connFD, "Logging you out now superman! Good bye!$", strlen("Logging you out now superman! Good bye!$"));
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
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    writeBytes = write(connFD, "Select account type\n1.Normal\n2.joint\n", strlen("Select account type\n1.Normal\n2.joint\n"));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_ACCOUNT_TYPE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account type response from client!");
        return false;
    }
    int acc_type = atoi(readBuffer);
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

            readBytes = read(accountFileDescriptor, &prevAccount, sizeof(struct Normal_Account));
            if (readBytes == -1)
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

        accountFileDescriptor = open(NORMAL_ACCOUNTS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        if (accountFileDescriptor == -1)
        {
            perror("Error while creating / opening Normal account file!");
            return false;
        }
        writeBytes = write(accountFileDescriptor, &newAccount, sizeof(struct Normal_Account));
        if (writeBytes == -1)
        {
            perror("Error while writing Normal Account record to file!");
            return false;
        }

        close(accountFileDescriptor);
        bzero(writeBuffer, sizeof(writeBuffer));
        sprintf(writeBuffer, "The newly created account's number is :%d", newAccount.accountNumber);
        strcat(writeBuffer, "\nRedirecting you to the main menu ...^");
        writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
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

            readBytes = read(accountFileDescriptor, &prevAccount, sizeof(struct Joint_Account));
            if (readBytes == -1)
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

        accountFileDescriptor = open(JOINT_ACCOUNTS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
        if (accountFileDescriptor == -1)
        {
            perror("Error while creating / opening Joint account file!");
            return false;
        }
        writeBytes = write(accountFileDescriptor, &newAccount, sizeof(struct Joint_Account));
        if (writeBytes == -1)
        {
            perror("Error while writing Joint Account record to file!");
            return false;
        }

        close(accountFileDescriptor);
        bzero(writeBuffer, sizeof(writeBuffer));
        sprintf(writeBuffer, "The newly created account's number is :%d", newAccount.accountNumber);
        strcat(writeBuffer, "\nRedirecting you to the main menu ...^");
        writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(writeBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
    else
    {
        return false;
    }
}

int add_customer(int connFD, int newAccountNumber)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

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

        readBytes = read(customerFileDescriptor, &previousCustomer, sizeof(struct Customer));
        if (readBytes == -1)
        {
            perror("Error while reading Customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFileDescriptor, F_SETLK, &lock);

        close(customerFileDescriptor);

        newCustomer.id = previousCustomer.id + 1;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "Enter the details for the customer\nWhat is the customer's name?");

    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_CUSTOMER_NAME message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading customer name response from client!");
        return false;
    }

    strcpy(newCustomer.name, readBuffer);
    // set account number
    newCustomer.account = newAccountNumber;

    strcpy(newCustomer.login, newCustomer.name);
    strcat(newCustomer.login, "-");
    sprintf(writeBuffer, "%d", newCustomer.id);
    strcat(newCustomer.login, writeBuffer);
    
    printf("\n%d\n", newCustomer.id);

    strcpy(newCustomer.password, AUTOGEN_PASSWORD);

    customerFileDescriptor = open(CUSTOMERS, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (customerFileDescriptor == -1)
    {
        perror("Error while creating / opening customer file!");
        return false;
    }
    writeBytes = write(customerFileDescriptor, &newCustomer, sizeof(newCustomer));
    if (writeBytes == -1)
    {
        perror("Error while writing Customer record to file!");
        return false;
    }

    close(customerFileDescriptor);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "The autogenerated login ID for the customer is : %s-%d\nThe autogenerated password for the customer is : %s", newCustomer.name, newCustomer.id, newCustomer.password);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error sending customer loginID and password to the client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return newCustomer.id;
}

bool delete_account(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    writeBytes = write(connFD, "Select account type\n1.Normal\n2.joint\n", strlen("Select account type\n1.Normal\n2.joint\n"));
    if (writeBytes == -1)
    {
        perror("Error writing ACCOUNT_TYPE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account type response from client!");
        return false;
    }
    int acc_type = atoi(readBuffer);


    if(acc_type==1){
        /***********************************************/
        /*NORMAL ACCOUNT*/
        /***********************************************/
        struct Normal_Account account;
        writeBytes = write(connFD, "What is the account number of the account you want to delete?", strlen("What is the account number of the account you want to delete?"));
        if (writeBytes == -1)
        {
            perror("Error writing ADMIN_DEL_ACCOUNT_NO to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from the client!");
            return false;
        }

        int accountNumber = atoi(readBuffer);

        int accountFileDescriptor = open(NORMAL_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1)
        {
            // Account record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer,"No account could be found for the given account number" );
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }

        int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Normal_Account), SEEK_SET);
        if (errno == EINVAL)
        {
            // Customer record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "No account could be found for the given account number");
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
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

        readBytes = read(accountFileDescriptor, &account, sizeof(struct Normal_Account));
        if (readBytes == -1)
        {
            perror("Error while reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        close(accountFileDescriptor);

        bzero(writeBuffer, sizeof(writeBuffer));
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

            writeBytes = write(accountFileDescriptor, &account, sizeof(struct Normal_Account));
            if (writeBytes == -1)
            {
                perror("Error deleting account record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            strcpy(writeBuffer, "This account has been successfully deleted\nRedirecting you to the main menu ...^");
        }
        else{
            // Account has some money ask customer to withdraw it
            strcpy(writeBuffer, "This account cannot be deleted since it still has some money\nRedirecting you to the main menu ...^");
        }
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing final DEL message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
    else if (acc_type==2){
        /***********************************************/
        /*Joint ACCOUNT*/
        /***********************************************/
        struct Joint_Account account;
        writeBytes = write(1, "What is the account number of the account you want to delete?", strlen("What is the account number of the account you want to delete?"));
        if (writeBytes == -1)
        {
            perror("Error writing ADMIN_DEL_ACCOUNT_NO to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from the client!");
            return false;
        }

        int accountNumber = atoi(readBuffer);

        int accountFileDescriptor = open(JOINT_ACCOUNTS, O_RDONLY);
        if (accountFileDescriptor == -1)
        {
            // Account record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "No account could be found for the given account number");
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }

        int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Joint_Account), SEEK_SET);
        if (errno == EINVAL)
        {
            // Customer record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "No account could be found for the given account number");
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
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

        readBytes = read(accountFileDescriptor, &account, sizeof(struct Joint_Account));
        if (readBytes == -1)
        {
            perror("Error while reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        close(accountFileDescriptor);

        bzero(writeBuffer, sizeof(writeBuffer));
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

            writeBytes = write(accountFileDescriptor, &account, sizeof(struct Joint_Account));
            if (writeBytes == -1)
            {
                perror("Error deleting account record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(accountFileDescriptor, F_SETLK, &lock);

            strcpy(writeBuffer, "This account has been successfully deleted\nRedirecting you to the main menu ...^");
        }
        else
        {
            // Account has some money ask customer to withdraw it
            strcpy(writeBuffer, "This account cannot be deleted since it still has some money\nRedirecting you to the main menu ...^");
        }
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing final DEL message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
    else{
        return false;
    }
    
}

#endif