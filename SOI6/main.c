#include "vsfs.h"

int main(int argc, char* argv[])
{
    /* 1 -> createDisk
     * 2 -> addFile
     * 3 -> deleteFile
     * 4 -> getFile
     * 5 -> viewDiskMap
     * 6 -> viewCatalogue
     * 7 -> removeDisk
     */
    int option, result;
    result = -42;
    if(argc < 2)
        printf("Nie poprawna liczba argumentow\n");
    else
    {
        option = atoi(argv[1]);

        if(option != 1)
            if(loadDiskSB()) { printf("Nieudany odczyt dysku"); return 0; }

        switch (option)
        {
            case 1:
                result = createDisk();
                if (result == 0)
                    printf("Dysk utworzono pomyslnie");
                else if (result == DISK_OPEN_ERROR)
                    printf("Nieudane utworzenie dysku\n");
                else if (result == SUPER_BLOCK_ERROR)
                    printf("Nieudane tworzenie Super Blocku\n");
                break;
            case 2:
                if(argc != 3)
                    printf("Niepoprawna liczba argumentow\n");
                else
                {
                    result = addFile(argv[2]);
                    if(result == FILE_OPEN_ERROR)
                        printf("Nieudane otworzenie pliku do skopiowania");
                    else if(result == DISK_OPEN_ERROR)
                        printf("Nieudane otworzenie dysku");
                    else if(result == NOT_UNIQUE_FILE_NAME)
                        printf("Plik o podanej nazwie istnieje na dysku");
                    else if(result == NOT_ENOUGH_SPACE)
                        printf("Brak miejsca na dysku");
                    else if(result == NEW_INODE_CREATION_ERROR)
                        printf("Błąd utworzenia pliku");
                    else 
                    	 printf("Plik dodano pomyślnie");
                }
                break;
            case 3:
                if(argc != 3)
                    printf("Niepoprawna liczba argumentow\n");
                else
                {
                    result = deleteFile(argv[2]);

                    if(result == DISK_OPEN_ERROR)
                        printf("Nieudane otworzenie dysku");
                    else if(result == CANNOT_FIND_FILE)
                        printf("Nie odanleziono pliku na dysku");
                    else
                        printf("Pomyslnie usunieto plik");
                }
                break;
            case 4:
                if(argc != 3)
                    printf("Niepoprawna liczba argumentow\n");
                else
                {
                    result = getFile(argv[2]);
                    if(result == DISK_OPEN_ERROR)
                        printf("Nieudane otworzenie dysku");
                    else if(result == CANNOT_FIND_FILE)
                        printf("Nie odnaleziono pliku");
                    else if(result == FILE_OPEN_ERROR)
                        printf("Nieudane utworzenie kopii pliku na dysku docelowym");
                    else
                        printf("Plik skopiowano pomyslnie");
                }

                if(result == CANNOT_FIND_FILE)
                    printf("Blad wyszukiwania pliku na dysku\n");
                else if(result == FILE_OPEN_ERROR)
                    printf("Blad tworzenia kopii pliku\n");
                    
                break;
            case 5:
                viewDiskMap();
                break;
            case 6:
                if (viewCatalogue() == DISK_OPEN_ERROR)
                    printf("Nieudane otworzenie dysku");
                 break;
            case 7:
                if(removeDisk() != 0)
                    printf("Usunięcie dysku nie powiodło się\n");
                else
                    printf("Usunięcie dysku zakończone pomyślnie\n");
                break;
            default:
                printf("Nie poprawna opcja");
        }
    }
    if(ptr_to_superBlock != NULL)
    {
        free(ptr_to_superBlock);
        ptr_to_superBlock = NULL;
    }
    return 0;
}
