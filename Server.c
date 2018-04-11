/**
 * Programa servidor en C para conectarse con un cliente java.
*/

///Directivas definidas
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Socket_Servidor.h"
#include "Socket.h"

///Variables definidas
#define PORT 3550
#define MAX_CLIENTES 10

///Funciones definidas
void nuevoCliente(int servidor, int *clientes, int *nClientes);
int obtenerMaximo(int *tabla, int n);
void compactaClaves(int * tabla, int *n);

///Estructura que contiene los datos del juego
struct Datos
{
    int puntuacion;
    int vidas;
    int velocidad;
    int estadoBunker;
};
//10.0.2.15


/**
 * Programa principal.
 * Crea un socket servidor y se mete en un select() a la espera de clientes.
 * Cuando un cliente se conecta, le atiende y lo agreag al select() y empieza
 * de nuevo.
 */
int main ()
{

    int socketServidor;///Descriptor del socket servidor.
    int socketCliente[MAX_CLIENTES];///Descriptores de sockets con clientes.
    int numeroClientes;///Numero de clientes conectados.
    fd_set descriptoresLectura;///Descriptores de lectura del select().

    int buffer; ///Buffer para leer de los sockets.
    int maximo; ///Numero descriptor mas grande
    int i;///Contador para ciclos o bucles

    ///Variables auxiliares de formato de red
    int auxiliar;
    int longitudCadena;
    char cadena[1000];

    ///Se abre el socket servidor con su respecitvo puerto. Avisa si hay algun
    ///problema o error
    socketServidor = Abre_Socket_Inet(PORT);
    if(socketServidor == -1)
    {
      perror("Error al abrir socket servidor");
      exit(-1);
    }

    ///Bucle infinito
    ///Se atiende las conexiones de los clientes y los mensajes enviados por los
    ///clientes conectados.
    while(1)
    {

        ///Se comprueba si algun cliente nuevo desea conectarse y se le admite.
        if(FD_ISSET(socketServidor, &descriptoresLectura))
            nuevoCliente(socketServidor, socketCliente, &numeroClientes);

        ///Se eliminan todos los clientes que hayan cerrado la conexion.
        compactaClaves(socketCliente, &numeroClientes);

        ///Se inicializan los descriptores de lectura.
        FD_ZERO (&descriptoresLectura);

        ///Se agrega al select() el socket servidor.
        FD_SET (socketServidor, &descriptoresLectura);

        ///Se agregan para el select() los sockets con los clientes ya conectados.
        for(i = 0; i < numeroClientes; i++)
            FD_SET (socketCliente[i], &descriptoresLectura);

        ///Se verifica el valor del descriptor mas grande. Si no hay ningun cliente,
        ///devolvera 0.
        maximo = obtenerMaximo(socketCliente, numeroClientes);
        if(maximo < socketServidor)
            maximo = socketServidor;

        ///Espera indefinida hasta que alguno de los descriptores tenga algo que
        ///decir: un nuevo cliente o un cliente ya conectado que envia un mensaje.
        select(maximo + 1, &descriptoresLectura, NULL, NULL, NULL);

        ///Se comprueba si algun cliente ya conectado envia algo.
        for(i = 0; i < numeroClientes; i++)
        {
            if(FD_ISSET (socketCliente[i], &descriptoresLectura))
            {
                ///Se lee lo enviado por el cliente y se escribe en pantalla
                if((Lee_Socket(socketCliente[i], (char *)&buffer, sizeof(int)) > 0))
                {
                    ///El entero recibido hay que transformalo de formato de red a
                    ///formato de hardware.
                    longitudCadena = ntohl(buffer);
                    ///Se lee la cadena enviada
                    Lee_Socket(socketCliente[i], cadena, longitudCadena);
                    printf("Cliente %d envia %s\n", i + 1, cadena);

                    ///Se envia datos a los clientes observadores
                    for(int j = 0; j < numeroClientes; j++)
                    {
                        if(i != j)
                        {
                            ///Se envia un entero con la longitud de la cadena (incluido
                            ///el \0 del final) y la cadena.

                            ///El entero que se envia por el socket hay que transformarlo
                            ///a formato de red
                            auxiliar = htonl(longitudCadena);

                            ///Se envia el entero transformado
                            Escribe_Socket(socketCliente[j], (char *)&auxiliar, sizeof(int));
                            printf("Servidor C: Enviado %d a Cliente Observador\n", longitudCadena -1);

                            ///Se envia la cadena recibida
                            Escribe_Socket(socketCliente[j], cadena, longitudCadena);
                            printf("Servidor C: Enviado %s a Cliente Observador\n", cadena);
                        }
                    }

                }
                else
                {
                    ///Se indica que el cliente ha cerrado la conexion y se marca
                    ///con -1 el descriptor para que compactaClaves() lo elimine.
                    printf("Cliente %d ha cerrado la conexion\n", i+1);
                    socketCliente[i] = -1;
                }
            }
        }
    }
    return 0;
}

/**
 * Crea un nuevo socket cliente.
 * Se le pasa el socket servidor y el arreglo de clientes, con el numero de clientes
 * ya conectados.
 */
void nuevoCliente(int servidor, int *clientes, int *nClientes)
{
    ///Acepta la conexion con el cliente, y la guarada en el arreglo.
    clientes[*nClientes] = Acepta_Conexion_Cliente(servidor);
    (*nClientes)++;

    ///Si se super el maximo de clientes, se cierra la conexion, se dejan como estaba
    ///y se retorna.
    if((*nClientes) >= MAX_CLIENTES)
    {
        close(clientes[(*nClientes) - 1]);
        (*nClientes)--;
        return;
    }
    //printf(*nClientes);

    ///Envia su numero de cliente al cliente.
    //Escribe_Socket(clientes[(*nClientes)-1], (char *)&aux, sizeof(int));

    ///Escribe en pantalla que ha aceptado al cliente y se retorna.
    //printf(("Aceptado cliente %d\n", *nClientes));
    printf("Aceptado cliente\n");
}


/**
* Funcion que devuelve el valor maximo en la tabla.
* Supone que los valores validos de la tbal son positivos y mayores que 0.
* Devuelve 0 si n es 0 o la tabla es NULL.
*/
int obtenerMaximo(int *tabla, int n)
{
    int i;
    int max;
    if((tabla == NULL) || (n < 1))
        return 0;
    max = tabla[0];
    for(i = 0; i < n; i++)
    {
        if(tabla[i] > max)
            max = tabla[i];
    }
    return max;
}

/**
* Buscar en el arreglo todas las posiciones con -1 y las elimina, copiando
* encima las posiciones siguientes.
*/
void compactaClaves(int *tabla, int *n)
{
    int i,j;
    if((tabla == NULL) || ((*n) == 0))
        return;
    j=0;
    for(i = 0; i < (*n); i++)
    {
        if(tabla[i] != -1)
        {
            tabla[j] = tabla[i];
            j++;
        }
    }
    *n = j;
}