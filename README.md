# Trabajo Práctico de la materia: 75.42 Taller de Programación I - FIUBA

# Introducción  

	Supongamos que hay dos computadoras, A y B, conectadas en red. Supongamos que en la computadora A hay un archivo F1 y en la B hay un archivo muy similar a F1 llamado F2.
	  
	*¿Cómo se puede hacer para que la computadora A pueda tener una copia de F2?*
	  
	Una solución trivial es simplemente conectarse a B y copiar todo el contenido de F2 en A. Pero si el canal de comunicación entre A y B es lento o el tamaño del archivo F2 es enorme, es una solución demasiado ineficiente. Una alternativa más inteligente consiste en enviar a A solo las diferencias entre F1 y F2. De esa manera A puede reconstruir F2 en su maquina a partir de F1 y las diferencias. Pero esto requiere que en la maquina B exista una copia de F1, de otro modo no podrá calcular las diferencias entre F1 y F2. El algoritmo de rsync [1] permite sincronizar las dos versiones, reconstruyendo F2 en la maquina de A a partir de F1 y de las diferencias entre estos, calculadas por la maquina B. 
	  
	La estrategia consiste primero en que A calcule una serie de checksums al archivo F1 y se los envíe a la  maquina B.

	En B, se utilizan estos checksums para saber qué partes del archivo F2 tiene en común con F1 y qué partes  son distintas calculando efectivamente las diferencias entre estos. Esta información se le envia a A quien reconstruye finalmente F2. En este trabajo práctico se implementará una versión reducida y simplificada del algoritmo para sincronizar  dos archivos. 

# Formato de Línea de Comandos

	Hay dos formas de ejecutar el programa: una en modo servidor que tendrá la versión actualizada de un  archivo en particular; 
	el otro, modo cliente, que tiene una versión desactualizada de un archivo y se  conectará al server para actualizarlo.


	Para ejecutar en modo servidor:   

	  **./tp  server  port**     

	Donde  port  es el puerto o servicio en donde escuchara el socket.

	Para ejecutar en modo cliente:    

	  **./tp client hostname port old_local_file new_local_file new_remote_file  block_size** 


	Los parámetros hostname y port  son el hostname/IP y el servicio/puerto del server remoto.  Donde  *old_local_file* es el archivo que se desea actualizar y que el cliente tiene acceso (es local a la  maquina); new_local_file es el nombre del archivo que el cliente va a crear (este no existe aún);  new_remote_file es el nombre del archivo que tiene la versión actualizada pero este archivo no se  encuentra en la maquina del cliente sino en la del server (es remoto). El parametro block_size es es tamaño en 

	bytes de los bloques usados en el cálculo del checksum.   

# Códigos de Retorno 

	El programa retorna 1  si hubo un error en los parametros. 

	En todo otra situación, retorna  0.

# Integrantes

- [Maria Eugenia Mariotti](https://github.com/emariotti3){:target="_blank"}
