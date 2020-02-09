# Productor-Consumidor-completo-mediante-paso-de-mesajes-MPI
Para ejecutar: mpirun -np "numero_de_procesos" ./prodcons_all

ejemplo: mpirun -np 14 ./prodcons_all

Alberto Robles Hernandez

 Sistemas concurrentes y Distribuidos.
 Práctica 3. Implementación de algoritmos distribuidos con MPI
 
 Flujo de paso de mensajes:

    1 Productor  n Distribuidores    1 Buffer    n Consumidores   1 Papelera   n Basureros

                 |  distribuidor |              |  consumidor |                | basurero |
                 |  ...          |              |  ...        |                |  ...     |
    Productor -> |  ...          | -> Buffer -> |  ...        | -> Papelera -> |  ...     |
                 |  ...          |              |  ...        |                |  ...     |
                 |  distribuidor |              |  consumidor |                | basurero |
