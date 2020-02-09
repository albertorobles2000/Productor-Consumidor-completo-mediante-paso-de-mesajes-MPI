// -----------------------------------------------------------------------------
// 
// Alberto Robles Hernandez
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
//  1 Productor  n Distribuidores    1 Buffer    n Consumidores   1 Papelera   n Basureros
//
//   		 |  distribuidor |  		|  consumidor |		       | basurero |
//               |  ...          |		|  ...        |		       |  ...     |
//  Productor -> |  ...	         | -> Buffer -> |  ...        | -> Papelera -> |  ...     |
//               |  ... 	 |		|  ...        |		       |  ...     |
//               |  distribuidor |		|  consumidor |		       | basurero |
//
// -----------------------------------------------------------------------------

/*******************************************************************************/
/* COMPILAR: mpicxx -std=c++11 -o prodcons_basurero prodcons_basurero.cpp ******/
/* EJECUTAR: mpirun -np 14 ./prodcons_basurero *********************************/
/*******************************************************************************/

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   id_productor           = 0,
   n_distribuidores       = 4,
   id_buffer              = n_distribuidores + 1,
   n_consumidores         = 5,
   id_papelera            = n_distribuidores + n_consumidores + 2, //+2 por el productor y el buffer
   n_basureros            = 2,
   num_procesos_esperado  = 14,
   num_items              = 4, //num_items tiene que ser multiplo de n_productores, n_consumidores y n_basureros
   tam_vector             = 10,
   tam_papelera           = 3,
   elementos_a_enviar     = 1,
   etiqueta_distribuidor  = 0,   
   etiqueta_consumidor    = 1,
   etiqueta_basurero      = 2;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio

int producir()
{
   static int contador = 0;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor ha producido valor " << contador << endl << flush;
   return contador ;
}

// ---------------------------------------------------------------------

void gestionar()
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
}

// ---------------------------------------------------------------------

void funcion_productor(){
	int solicitud;
	MPI_Status  estado ;

	for ( unsigned int i= 0 ; i < num_items ; i++ )
   {
      // producir valor
      int valor_prod = producir();
	
	  MPI_Recv ( &solicitud, elementos_a_enviar, MPI_INT, MPI_ANY_SOURCE, etiqueta_distribuidor, MPI_COMM_WORLD,&estado );
	  cout << "Productor va a enviar el valor " << valor_prod << " a distribuidor " << estado.MPI_SOURCE << endl << flush;

      MPI_Ssend( &valor_prod, elementos_a_enviar, MPI_INT, estado.MPI_SOURCE, etiqueta_distribuidor, MPI_COMM_WORLD );
   }
}

// ---------------------------------------------------------------------

void funcion_distribuidor(int proceso){
	int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/n_distribuidores; i++ )
   {
      MPI_Ssend( &peticion,  elementos_a_enviar, MPI_INT, id_productor, etiqueta_distribuidor, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, elementos_a_enviar, MPI_INT, id_productor, etiqueta_distribuidor, MPI_COMM_WORLD,&estado );
      cout << "Distribuidor " << proceso << " ha recibido valor " << valor_rec << endl << flush ;
      gestionar();
	  cout << "Distribuidor " << proceso << " va a enviar el valor " << valor_rec << " al buffer" << endl << flush ;
	  MPI_Ssend( &valor_rec,  elementos_a_enviar, MPI_INT, id_buffer, etiqueta_distribuidor, MPI_COMM_WORLD);	
   }
}

// ---------------------------------------------------------------------

void funcion_buffer(){
	int        buffer[tam_vector],       // FIFO
              valor,                     // valor recibido o enviado
              primera_libre       = 0,   // índice de primera celda libre
              primera_ocupada     = 0,   // índice de primera celda ocupada
              num_celdas_ocupadas = 0,   // número de celdas ocupadas
              etiqueta_emisor_aceptable; // identificador de emisor aceptable
   MPI_Status estado ;                   // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               	    // si buffer vacío
         etiqueta_emisor_aceptable = etiqueta_distribuidor; // $~~~$ solo prod.
      else if ( num_celdas_ocupadas == tam_vector )         // si buffer lleno
         etiqueta_emisor_aceptable = etiqueta_consumidor;   // $~~~$ solo cons.
      else                                                  // si no vacío ni lleno
         etiqueta_emisor_aceptable = MPI_ANY_TAG ;          // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, elementos_a_enviar, MPI_INT, MPI_ANY_SOURCE, etiqueta_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
      {
         case etiqueta_distribuidor: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor << endl << flush;
            break;

         case etiqueta_consumidor: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada] ;
            primera_ocupada = (primera_ocupada+1) % tam_vector ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl << flush;
            MPI_Ssend( &valor, elementos_a_enviar, MPI_INT, estado.MPI_SOURCE , etiqueta_consumidor, MPI_COMM_WORLD);
            break;
      }
   }
}

// ---------------------------------------------------------------------

void consumir( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor ha consumido valor " << valor_cons << endl << flush ;
}

// ---------------------------------------------------------------------

void funcion_consumidor(int proceso){
	int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/n_consumidores; i++ )
   {
      MPI_Ssend( &peticion,  elementos_a_enviar, MPI_INT, id_buffer, etiqueta_consumidor, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, elementos_a_enviar, MPI_INT, id_buffer, etiqueta_consumidor, MPI_COMM_WORLD,&estado );
      cout << "Consumidor " << proceso << " ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec );
	  cout << "Consumidor " << proceso << " Ya ha acabao va a tirar a la basura el valor " << valor_rec << endl << flush ;
	  MPI_Ssend( &valor_rec,  elementos_a_enviar, MPI_INT, id_papelera, etiqueta_consumidor, MPI_COMM_WORLD);	
   }
}

// ---------------------------------------------------------------------

void funcion_papelera(){
	
    int 	buf[tam_papelera], //LIFO
	 		usados = 0,
	 		valor = -1,
		 	etiqueta;
	MPI_Status estado;
	for(int i=0; i<num_items*2; i++){
		
		// 1. determinar si puede enviar solo basurero., solo cons, o todos

		if(usados == tam_papelera)
			etiqueta = etiqueta_basurero;
		else
		if(usados == 0)
			etiqueta = etiqueta_consumidor;
		else
			etiqueta = MPI_ANY_TAG;

		 // 2. recibir un mensaje del emisor o emisores aceptables
		
		MPI_Recv( &valor, elementos_a_enviar, MPI_INT, MPI_ANY_SOURCE, etiqueta, MPI_COMM_WORLD, &estado );

		// 3. procesar el mensaje recibido

		switch( estado.MPI_TAG ){

			case etiqueta_consumidor: // si ha sido el consumidor: insertar en la papelera
				buf[usados] = valor;
				usados++;
				cout << "Han tirado a la papelera el valor " << valor << endl << flush;
				break;

			case etiqueta_basurero:  // si ha sido el basurero: extraer y enviarle
				usados--;
				valor = buf[usados];
				MPI_Ssend(&valor, elementos_a_enviar, MPI_INT, estado.MPI_SOURCE , etiqueta_basurero, MPI_COMM_WORLD);
				break;	
		}
	}
}

// ---------------------------------------------------------------------

void reciclar( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Basurero ha reciclado valor " << valor_cons << endl << flush ;
}

// ---------------------------------------------------------------------

void funcion_basurero(int proceso){
	int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/n_basureros; i++ )
   {
      MPI_Ssend( &peticion,  elementos_a_enviar, MPI_INT, id_papelera, etiqueta_basurero, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, elementos_a_enviar, MPI_INT, id_papelera, etiqueta_basurero, MPI_COMM_WORLD,&estado );
      cout << "Basurero " << proceso << " ha cogido de la papelera el valor " << valor_rec << endl << flush ;
      reciclar( valor_rec );
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio == id_productor )
         funcion_productor();
      else if ( id_propio <= n_distribuidores )
         funcion_distribuidor(id_propio);
      else if ( id_propio == id_buffer )
         funcion_buffer();
      else if (id_propio <= n_distribuidores + n_consumidores + 1)
	 funcion_consumidor(id_propio);
      else if (id_propio == id_papelera)
	 funcion_papelera();
      else
	 funcion_basurero(id_propio);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}

