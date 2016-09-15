#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> // for close

#define NO_CLIENTES 1
#define NO_MENSAGENS 5
#define TAM_SENDBUFF 140
#define TAM_RECVBUFF 480
#define TAM_USUARIO 11
#define TAM_MENSAGEM 50
#define TAM_NUMERO_TELEFONE 14
#define TAM_ENDERECO_IP 10

//thread servidor
pthread_t thread_servidor;

//mutex
pthread_mutex_t mutex;

typedef struct {
	int conexao;
	char endereco_ip_recebido[TAM_ENDERECO_IP];
}estrutura_enviada_thread;

struct cadastros{
	char telefone_cliente[TAM_NUMERO_TELEFONE];
	char endereco_ip_cliente[TAM_ENDERECO_IP];
	int porta_cliente;
	struct cadastros *p_proximo_cadastro;
}cadastros;

struct cadastros *p_inicio_cadastros, *p_novo_cadastro, *p_auxiliar, *p_anterior;

int numero_cadastros_online = 0;

void *Servidor(void *args);

/*
 * Servidor TCP
 */
int main(argc, argv)
int argc;
char **argv;
{
	pthread_t thread_servidor;
	unsigned short port;            
	struct sockaddr_in client; 
	struct sockaddr_in server; 
	int socket_servidor;                     /* Socket para aceitar conexões       */
	int novo_socket;                    /* Socket conectado ao cliente        */
	int namelen;
	int ts;
	estrutura_enviada_thread mensagem_thread;

	/*
	* O primeiro argumento (argv[1]) é a porta
	* onde o servidor aguardará por conexões
	*/
	if(argc != 2)
	{
		fprintf(stderr, "Use: %s porta\n", argv[0]);
		exit(1);
	}

	port = (unsigned short) atoi(argv[1]);

	//Inicia a lista
	p_inicio_cadastros = (struct cadastros *) NULL;

	/*
	* Cria um socket TCP (stream) para aguardar conexões
	*/
	if((socket_servidor = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket()");
		exit(2);
	}

	/*
	* Define a qual endereço IP e porta o servidor estará ligado.
	* IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
	* os endereços IP
	*/
	server.sin_family = AF_INET;   
	server.sin_port   = htons(port);       
	server.sin_addr.s_addr = INADDR_ANY;

	/*
	* Liga o servidor à porta definida anteriormente.
	*/
	if(bind(socket_servidor, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Bind()");
		exit(3);
	}

	if(listen(socket_servidor, NO_CLIENTES) != 0)
	{
		perror("Listen()");
		exit(4);
	}

	//pthread_mutex_init(&mutex, NULL);

	while(1)
	{
		namelen = sizeof(client);

		if((novo_socket = accept(socket_servidor, (struct sockaddr *)&client, &namelen)) == -1)
		{
			perror("Accept()");
			exit(5);
		}

		mensagem_thread.conexao = novo_socket;
		strcpy(mensagem_thread.endereco_ip_recebido, inet_ntoa(client.sin_addr));

		ts = pthread_create(&thread_servidor, NULL, Servidor, (void *) &mensagem_thread);

		if(ts)
		{
			printf("ERRO: impossivel criar um thread servidor\n");
			exit(-1);
		}

		pthread_detach(thread_servidor);
	}

	close(socket_servidor);
	exit(0);
}

void *Servidor(void *args)
{
	estrutura_enviada_thread novo_socket;
	char sendbuf[TAM_SENDBUFF];              
	char recvbuf[TAM_RECVBUFF];
	int verificar_status_cliente;
	char opcao;
	int porta;
	int tam;
	char endereco_ip[TAM_ENDERECO_IP];
	char numero_telefone[TAM_NUMERO_TELEFONE];
	char numero_telefone_comparado[TAM_NUMERO_TELEFONE];

	novo_socket = *(estrutura_enviada_thread *)args;

	strcpy(endereco_ip, novo_socket.endereco_ip_recebido);

	printf("\nThread: %d\n", novo_socket.conexao);

	//Recebendo o status do cliente e depois armazenando na variavel verificar_status_cliente
	if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	sscanf(recvbuf, "%d %s", &porta, numero_telefone);

	printf("\nPorta: %d\nTelefone: %s\n", porta, numero_telefone);

	p_auxiliar = p_inicio_cadastros;

	//Procura na lista se o telefone ja esta cadastrado no servidor
	if(p_auxiliar != NULL)
	{
		do
		{
			if(strcmp(p_auxiliar->telefone_cliente, numero_telefone) == 0)
			{
				//Se a porta recebida for igual à porta cadastrada, marcar a variavel de estado como usuario ja cadastrado
	 			if(p_auxiliar->porta_cliente == porta)
				{
					verificar_status_cliente = 1;
				}
				//Se a porta recebida for diferente da porta cadastrada, atualizar a porta e marcar a variavel de estado como usuario ja cadastrado
				else
				{
					p_auxiliar->porta_cliente = porta;
					printf("\nPorta atualizada!\n");

					verificar_status_cliente = 1;
				}

				break;
			}
			else
			{
				verificar_status_cliente = 0;
			}

			//Passa para o proximo da lista
			p_auxiliar = p_auxiliar->p_proximo_cadastro;
		}while(p_auxiliar != NULL);	
	}

	switch(verificar_status_cliente)
	{
		//0 - Cliente nao cadastrado
		case 0:		printf("\nNovo cadastro!\n");

				//Criando um novo cadastro para ser adicionado à lista de cadastros (online)
				p_novo_cadastro = (struct cadastros *)malloc(sizeof(struct cadastros));

				//Incrementar numero de pessoas online (numero de pessoas na lista) 
				numero_cadastros_online++;

				//Armazenando dados recebidos (numero de telefone, endereco IP e porta)
				strcpy(p_novo_cadastro->telefone_cliente, numero_telefone);
				strcpy(p_novo_cadastro->endereco_ip_cliente, endereco_ip);
				p_novo_cadastro->porta_cliente = porta;

				printf("\nNovo telefone: %s\n", p_novo_cadastro->telefone_cliente);
				printf("\nNovo endereco IP: %s\n", p_novo_cadastro->endereco_ip_cliente);
				printf("\nNova porta: %d\n", p_novo_cadastro->porta_cliente);

				//O novo cadastro aponta para o inicio da lista (ultimo cadastrado)
				p_novo_cadastro->p_proximo_cadastro = p_inicio_cadastros;
				//O novo cadastro torna-se o inicio da lista
				p_inicio_cadastros = p_novo_cadastro;

				//system("clear");
		
				break;

		//1 - Cliente cadastrado (online)
		case 1:		printf("\nCliente ja cadastrado!\n");

				strcpy(sendbuf, "Esperando requisicao");

				if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
				{
					perror("Send()");
					exit(5);
				}

				//Recebendo opcao
				if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
				{
					perror("Recv()");
					exit(6);
				}

				if(strcmp(recvbuf, "Consultar") == 0)
				{
					p_auxiliar = p_inicio_cadastros;

					printf("\nUsuarios cadastrados:\n");

					if(p_auxiliar != NULL)
					{
						do
						{
							printf("\nTelefone: %s\n", p_auxiliar->telefone_cliente);
							printf("Endereco IP: %s\n", p_auxiliar->endereco_ip_cliente);
							printf("Porta: %d\n", p_auxiliar->porta_cliente);

							//sprintf(sendbuf, "%s %s %d", p_auxiliar->telefone_cliente, p_auxiliar->endereco_ip_cliente, p_auxiliar->porta_cliente);
							
							sprintf(sendbuf, "%s", p_auxiliar->telefone_cliente);

							tam = strlen(sendbuf);

							printf("\nTamanho: %d\n", tam);

							//Mandando o contato, um por vez
							if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
							{
								perror("Send()");
								exit(5);
							}

							//Recebendo confirmacao de envio do contato
							if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
							{
								perror("Recv()");
								exit(6);
							}

							//Passa para o proximo da lista
							p_auxiliar = p_auxiliar->p_proximo_cadastro;

							printf("\n");
						}while(p_auxiliar != NULL);	
					}

					strcpy(sendbuf, "Terminou");

					//Avisando que todos os contatos da lista foram mandados
					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}

					printf("\nMandei Terminou!\n");
				}
				else if(strcmp(recvbuf, "Porta e Endereco IP") == 0)
				{
					strcpy(sendbuf, "Cliente?");
					
					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}

					if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
					{
						perror("Recv()");
						exit(6);
					}

					printf("\nTelefone recebido recvbuf: %s\n", recvbuf);

					sscanf(recvbuf, "%s", numero_telefone_comparado);

					printf("\nTelefone recebido numero_telefone_comparado: %s\n", numero_telefone_comparado);

					int contador = 0;

					p_auxiliar = p_inicio_cadastros;

					//
					if(p_auxiliar != NULL)
					{
						do
						{
							printf("\nTelefone da lista: %s\n", p_auxiliar->telefone_cliente);
							
							if(strcmp(p_auxiliar->telefone_cliente, numero_telefone_comparado) == 0)
							{
								printf("\nEncontrou %s\n", p_auxiliar->telefone_cliente);

								sprintf(sendbuf, "%s %d", p_auxiliar->endereco_ip_cliente, p_auxiliar->porta_cliente);

								break;
							}
							else
							{
								contador++;
								printf("\nEntrou aqui %d vezes!\n", contador);

								strcpy(sendbuf, "Cliente nao esta online");
							}

							//Passa para o proximo da lista
							p_auxiliar = p_auxiliar->p_proximo_cadastro;
						}while(p_auxiliar != NULL);	
					}
					else
					{
						printf("\nNao ha clientes online");

						strcpy(sendbuf, "Nao ha usuarios online");
					}

					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}
				}
				else if(strcmp(recvbuf, "Portas e Enderecos IP") == 0)
				{
					strcpy(sendbuf, "Clientes?");
					
					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}

					while(1)
					{
						if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
						{
							perror("Recv()");
							exit(6);
						}

						p_auxiliar = p_inicio_cadastros;

						printf("\nTelefone recebido recvbuf: %s\n", recvbuf);

						sscanf(recvbuf, "%s", numero_telefone_comparado);

						printf("\nTelefone recebido numero_telefone_comparado: %s\n", numero_telefone_comparado);

						if(strcmp(recvbuf, "Todas as localizacoes ja foram solicitadas") == 0)
						{
							break;
						}
						else
						{
							if(p_auxiliar != NULL)
							{
								do
								{
									if(strcmp(p_auxiliar->telefone_cliente, numero_telefone_comparado) == 0)
									{
										printf("Encontrou %s\n", p_auxiliar->telefone_cliente);

										sprintf(sendbuf, "%s %d", p_auxiliar->endereco_ip_cliente, p_auxiliar->porta_cliente);

										break;
									}
									else
									{
										strcpy(sendbuf, "Cliente nao esta online");
									}

									//Passa para o proximo da lista
									p_auxiliar = p_auxiliar->p_proximo_cadastro;
								}while(p_auxiliar != NULL);	
							}
							else
							{
								printf("Nao ha clientes online");

								strcpy(sendbuf, "Nao ha usuarios online");
							}

							if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
							{
								perror("Send()");
								exit(5);
							}
						}
					}
				}
				else if(strcmp(recvbuf, "Sair") == 0)
				{
					strcpy(sendbuf, "Irei desconecta-lo");

					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}

					if(recv(novo_socket.conexao, recvbuf, sizeof(recvbuf), 0) == -1)
					{
						perror("Recv()");
						exit(6);
					}

					printf("\nNumero para ser desconectado: %s\n", recvbuf);

					p_auxiliar = p_inicio_cadastros;

					p_anterior = NULL;

					sscanf(recvbuf, "%s", numero_telefone_comparado);

					printf("\nVariavel numero_telefone_comparado: %s\n", numero_telefone_comparado);

					while(p_auxiliar != NULL)
					{	
						if(strcmp(p_auxiliar->telefone_cliente, numero_telefone_comparado) == 0)
						{
							printf("\nEncontrou %s\n", p_auxiliar->telefone_cliente);

							sprintf(sendbuf, "Desconectando %s", p_auxiliar->telefone_cliente);

							break;
						}
						else
						{
							strcpy(sendbuf, "Cliente nao conectado ao servidor");
						}

						p_anterior = p_auxiliar;
						p_auxiliar = p_auxiliar->p_proximo_cadastro;
					}

					if(p_auxiliar == NULL)
					{
						strcpy(sendbuf, "Cliente nao conectado ao servidor");
					}

					if(p_anterior != NULL)
					{
						p_anterior->p_proximo_cadastro = p_auxiliar->p_proximo_cadastro;
					}
					else
					{
						p_inicio_cadastros = p_auxiliar->p_proximo_cadastro;
					}

					free(p_auxiliar);

					if(send(novo_socket.conexao, sendbuf, strlen(sendbuf)+1, 0) < 0)
					{
						perror("Send()");
						exit(5);
					}
				}

				break;

		default:	break;
	}

	close(novo_socket.conexao);

	pthread_exit(NULL);
}
