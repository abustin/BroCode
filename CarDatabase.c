#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int size;
} DatabaseInfo;

typedef struct {
    int red;
    int green;
    int blue;
} Color;

typedef struct {
    char make[16];
    char model[16];
    short year;
    short numDoors;
    Color color;
} Car;

void printColor(Color c) {
    printf("The color is red:%d, green:%d, blue:%d\n", c.red, c.green, c.blue);
}

void printCar(Car *car) {
    printf("a %d %s %s\n", car->year, car->make, car->model);
    printColor(car->color);
    printf("-----------------------------------------\n");
}

void searchCarByMake(char *carMake) {

    int idx =0;
    Car car;
    FILE *fd = fopen("cars.db","r");
    
    while(1) {
        fseek(fd,sizeof(Car)*idx,SEEK_SET);
        size_t numRead = fread(&car,sizeof(Car),1,fd);
        
        if (numRead == 0) {
            printf("Search finished at idx %i\n", idx);
            break;
        } else if (strstr(car.make,carMake) != NULL) {
            printf("Found ..");
            printCar(&car);
        }

        idx++;
    }
    fclose(fd); 

}

void listAllCars() {

    printf("Listing all the cars!\n");
    printf("-----------------------------------------\n");

    int idx = 0;
    Car car;
    FILE *fd = fopen("cars.db","r");
    
    while(fd && 1) {
        fseek(fd,sizeof(Car)*idx,SEEK_SET);
        size_t numRead = fread(&car,sizeof(Car),1,fd);
        
        if (numRead == 0) {
            break;
        } else {
            printCar(&car);
        }
        idx++;

    }

    printf("\nFinished!\n\n");
    
    fclose(fd); 

}


void readCar(int idx) {
    Car car;
    FILE *fd = fopen("cars.db","r+");
    fseek(fd,sizeof(Car)*idx,SEEK_SET);
    size_t numRead = fread(&car,sizeof(Car),1,fd);
    if (numRead == 1) {
        printCar(&car);    
    } else {
        printf("Wasn't able to find idx %i in car data\n", idx);
    }
    
    fclose(fd); 
    
}

void readDatabaseInfo(DatabaseInfo *info) {
    
    FILE *fd = fopen("cars.db","r");
    size_t numRead = 0;
    if (fd!=NULL) {
        fseek(fd,-sizeof(DatabaseInfo),SEEK_END);
        numRead = fread(info,sizeof(DatabaseInfo),1,fd);
    }
    if (numRead == 0) {
        info->size = 0;
    }
    
    fclose(fd); 
    
}

void writeDatabaseInfo(DatabaseInfo *info) {

    printf("Updating database size to %i\n", info->size);
    
    FILE *fd = fopen("cars.db","r+");
    fseek(fd,sizeof(Car)*info->size,SEEK_SET);
    size_t numRead = fwrite(info,sizeof(DatabaseInfo),1,fd);
    fclose(fd); 
    
}

void writeCar(DatabaseInfo *info, Car* car) {

    printf("\nAdding .. ");
    printCar(car);

    FILE *fd = fopen("cars.db","r+");
    if (fd == NULL) {
        fd = fopen("cars.db","w+");
    }
    fseek(fd,sizeof(Car)*info->size,SEEK_SET);
    fwrite(car,sizeof(Car),1,fd);
    fclose(fd);

    // Update database info
    info->size++;
    writeDatabaseInfo(info);
}


int main(void) {

    // Create basic values on the stack

    // We'll use this to keep info about the database
    // We'll also write the info to the end of out database file
    DatabaseInfo info; 


    // We'll reuse this myCar object when adding new car info to the database
    Car myCar;


    // We'll ask the user for a command such as list/add/search. `buff` will store that command
    char buff[10];


    // Send `readDatabaseInfo` a pointer to our info object.  
    // If `readDatabaseInfo` is successful, it will populate the size of our info struct
    readDatabaseInfo(&info);

    
    printf ("Welcome to the Car Database!\n");
    

    if (info.size ==0) {
        printf("\nDatabase is currently Empty\n");
    } else {
        printf("\nDatabase currently has %i cars\n", info.size);
    }


    // Loop forever!! (or until we say `break`)
    while(1) {

        printf ("\n==========================\n");
        printf ("What would you like to do?\n");
        printf ("add, list, search, exit\n");
        printf ("==========================\n");

        scanf("%s",buff); // wait and get the command the user entered

        if (strcmp("add",buff) == 0) { // user said `add`

            printf ("Enter Car Make?\n");
            scanf("%s",&myCar.make[0]);  // Each scanf will copy the users input into our myCar struct
            

            printf ("Enter Car Model?\n");
            scanf("%s",&myCar.model[0]);

            printf ("Enter Car Year?\n");
            scanf("%hu",&myCar.year);

            printf ("Enter Number of Doors?\n");
            scanf("%hu",&myCar.numDoors);
            
            printf ("Color red? [0-255]\n");
            scanf("%i",&myCar.color.red);
            printf ("Color green? [0-255]\n");
            scanf("%i",&myCar.color.green);
            printf ("Color blue? [0-255]\n");
            scanf("%i",&myCar.color.blue);

            // When we've gotten all the car info, we'll send a pointer of myCar to writeCar
            // We'll also send a pointer to the database info so we can update the database size
            writeCar(&info, &myCar);

        } else if (strcmp("search", buff) == 0) { // user said `search`

            if (info.size > 0) {

                printf ("Car Make?\n");
                memset (buff,0,sizeof(buff));
                scanf("%s",buff);
                searchCarByMake(buff);
            } else {
                printf("\nDatabase is currently Empty\n");
            }
            
        } else if (strcmp("list", buff) == 0) { // user said `list`

            listAllCars();
            
        } else if (strcmp("exit", buff) == 0) { // user said `exit`
            
            printf("Bye\n");
            break;

        } else { // we're not mind readers

            printf("Sorry, I didn't understand your command :(\n");

        }

    }
 
    return 0;
}