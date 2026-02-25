#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define SEGUNDOS_2000_A_UNIX 946684800
#define PATH_MAX 4096

typedef struct
{ // Estructura donde se almacenarán los datos de cada estación igualmente estructurados en el archivo .bin
    __uint32_t id_estacion;
    __uint16_t presion;
    __int16_t temp;
    __uint16_t precip_caidas;
    __uint8_t hum;
    __uint32_t time;
} estacion_met;

int main(int argc, char *argv[])
{
    estacion_met estacion;            // Creo la estructura donde guardar los datos a leer
    bool archivo_csv_default = false; // Bandera booleana para controlar si se explicitó el nombre deseado para el archivo .csv
    bool cambio_directorio = false;
    char *directorio;      // Ruta de directorio
    char *archivo_csv;     // Cadena para guardar la ruta destino del archivo .csv
    char *archivo_binario; // Cadena para guardar la ruta origen del archivo .bin
    FILE *bin;             // Manejador de archivo para el archivo binario
    FILE *csv;              // Manejador de archivo para el archivo .csv
    char cwd[PATH_MAX];    // Array para almacenar el directorio actual

    if (argv[1] && strcmp(argv[1], "-cd") == 0)
    { // Verifico que argv[1] sea no NULL y que sean iguales este con "-cd" (strcmp daría 0)
        cambio_directorio = true;
    }
    if (cambio_directorio) // Caso donde se solicito "-cd"
    {
        directorio = malloc(strlen(argv[2]) + 2); // solicito memoria dinámica necesaria para que quepa argv[2] + '/' y '\0'
        strcpy(directorio, argv[2]);              // Copio argv[2] en directorio.
        size_t len = strlen(directorio);          // Calculo la congitud de sirectorio (con \0)
        if (directorio[len - 1] != '/')           // Chequeo si se introdujo el '/' al final del argv[2].
        {
            directorio[len] = '/';      // Agrego '/'
            directorio[len + 1] = '\0'; // Agrego el carácter de fin de cadena
        }
        if (argc < 4)
        { // Lógica de control de errores por pasaje de argumentos insuficientes
            perror("No se especificó la ruta del archivo bin origen");
            free(directorio);
            return 1;
        }
        else if (argc == 4)
        {
            printf("El archivo CSV se guardará en el directorio del programa como estaciones_met.csv\n");
            archivo_csv_default = true;
        }

        archivo_binario = malloc(strlen(directorio) + strlen(argv[3]) + 1); // Genero la cadena que guardará la ruta de origen del archivo binario,
                                                                            // inicializandola reservando memoria dinamica apartir del directorio y lo explicitado en el argumento 1
        strcpy(archivo_binario, directorio);                                // Concateno el directorio y lo pasado por el usuario en una sola cadena
        strcat(archivo_binario, argv[3]);

        free(directorio); // libero la memoria dinámica solicitada.

        if (archivo_csv_default)
        { // Lógica para decidir el nombre con el cual guardar el .csv (default o por usuario)
            archivo_csv = malloc(strlen("estaciones_met.csv") + 1);
            strcpy(archivo_csv, "estaciones_met.csv");
        }
        else
        {
            archivo_csv = malloc(strlen(argv[4]) + strlen(".csv") + 1);
            strcpy(archivo_csv, argv[4]);
            strcat(archivo_csv, ".csv");
        }
    }
    else // Caso donde no se solicite "-cd"
    {
        if (argc < 2)
        { // Lógica de control de errores por pasaje de argumentos insuficientes
            perror("No se especificó el archivo .bin origen");
            return 1;
        }
        else if (argc == 2)
        {
            printf("El archivo CSV se guardará en el directorio del programa como estaciones_met.csv\n");
            archivo_csv_default = true;
        }

        archivo_binario = malloc(strlen(argv[1]) + 1); // Genero la cadena que guardará la ruta de origen del archivo binario
        strcpy(archivo_binario, argv[1]);

        if (archivo_csv_default)
        { // Lógica para decidir el nombre con el cual guardar el .csv (default o por usuario)
            archivo_csv = malloc(strlen("estaciones_met.csv") + 1);
            strcpy(archivo_csv, "estaciones_met.csv");
        }
        else
        {
            archivo_csv = malloc(strlen(argv[2]) + strlen(".csv") + 1);
            strcpy(archivo_csv, argv[2]);
            strcat(archivo_csv, ".csv");
        }
    }

    bin = fopen(archivo_binario, "rb"); // Abro el archivo binario (con lo explicitado previamente) en modo lectura especifica para binario
    if (bin == NULL)
    { // Lógica para error en la apertura del archivo binario
        perror("Error al abrir archivo_binario");
        free(archivo_binario); // Debo liberar la memoria din. previamente solicitada
        return 1;
    }

    csv = fopen(archivo_csv, "w"); 
    if (csv == NULL)                     // Lógica por si no se logra abrir el csv
    {
        perror("Error al abrir el archivo binario origen");
        free(archivo_csv);
        return 1;
    }

    fprintf(csv, "%s,%s,%s,%s,%s,%s,%s\n", "ID Estación", "Fecha", "Hora", "Temperatura", "Presión", "Precipitaciones", "Humedad"); // Escribo el header de las columnas en el .csv

    while (fread(&estacion, sizeof(estacion_met), 1, bin)) // Lógica de escritura del .csv mientras la lectura del binario no finalice (fread==0)
    {
        // A fread le paso la direccion de memoria de estación (para que la modifique),
        // el tamaño del bloque a leer (la suma en bytes de todos los datos que la constituyen),
        // hago que lea 1 solo bloque por vez (osea una estación entera), y el archivo origen (le paso el manejador binario)
        // La copia de la lectura se distribuye correctamente en cada dato de la estructura unicamente por que
        // ésta está ordenada identicamente a como están ordenados los datos en el archivo binario

        // Convierto los datos leidos  guardados en la estructura (a que estan guardads como enteros aunque representan números reales)
        float temperatura = estacion.temp / 10.0;
        float presion = estacion.presion / 10.0;
        float precip = estacion.precip_caidas / 10.0;

        // Convierto los segundos leidos al dato temporal requerido usando localtime y la estructura tm de c
        __time_t tiempo_rel_UNIX = estacion.time + SEGUNDOS_2000_A_UNIX; // Uso el tipo de dato '__time_t', propio de "time.h", el cual almacena en tipo int
                                                                         // los segundos pasados desde la época UNIX ( de enero de 1970 a las 00:00:00 UTC).
                                                                         // de esta manera hago relativa la cantidad de segundos del binario (contados desde las 00:00 hs del 01/01/2000)
                                                                         // a la época UNIX
        struct tm *tiempo = localtime(&tiempo_rel_UNIX);                 // Uso la estructura 'tm' (propia también de "time.h"), para crear la estructura tiempo, ya que descompone un valor 'time_t'
                                                                         // en componentes como año (tm_year), mes (tm_mon), día (tm_mday o tm_wday o tm_yday), hora, minutos, segundos, etc.
                                                                         // Uso la funcion 'localtime' pasandole el valor relativo de los segundos leidos a la epoca UNIX, ya que esta función
                                                                         // se encarga de convertir un valor 'time_t', en una estructura 'tm' que contenga la fecha y la hora de acuerdo con la hora local.

        // Escribo el .csv (las correciones en tm_mon y tm_year se deben a que, en tm_mon los meses comienzan desde el mes 0;  en tm_year, los años son relativos al 1900)
        fprintf(csv, "%u,%02d/%02d/%04d,%02d:%02d:%02d,%2.1f °C,%4.1f mbar,%3.1f mm,%u%%\n",
                estacion.id_estacion, tiempo->tm_mday, tiempo->tm_mon + 1, tiempo->tm_year + 1900,
                tiempo->tm_hour, tiempo->tm_min, tiempo->tm_sec, temperatura, presion, precip, estacion.hum);
    }

    fclose(bin); // Cierro el archivo binario y csv
    fclose(csv);

    free(archivo_csv); // Libero la memoria dinámica solicitada
    free(archivo_binario);

    getcwd(cwd, sizeof(cwd)); // Obtengo el directorio de ejecución del programa
    printf("El archivo destino se encuentra en el directorio: %s\n", cwd);

    return 0;
}