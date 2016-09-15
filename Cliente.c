#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio_ext.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h> // for close

#define NO_CLIENTES 1
#define TAM_SENDBUFF 140
#define TAM_RECVBUFF 480
#define TAM_NUMERO_TELEFONE 14
#define TAM_NOME_GRUPO 10
#define TAM_NOME_ARQUIVO_CONTATOS 25
#define TAM_NOME_ARQUIVO_GRUPO 25
#define TAM_NOME_ARQUIVO_MENSAGENS 25
#define TAM_MENSAGEM 140
#define TAM_ENDERECO_IP 10

//thread cliente e servidor
pthread_t thread_cliente;
pthread_t thread_servidor;

char sendbuf[TAM_SENDBUFF];              
char recvbuf[TAM_RECVBUFF];
char numero_telefone[TAM_NUMERO_TELEFONE];

FILE *fp_contatos, *fp_nome_grupos;
char arquivo_contatos[TAM_NOME_ARQUIVO_CONTATOS], arquivo_nome_grupos[TAM_NOME_ARQUIVO_GRUPO];
int socket_servidor;
int numero_grupos = 0;

int estabelecer_conexao(struct sockaddr_in server);
void criar_grupo();
void mandar_mensagem_para_grupo(char nome_grupo[TAM_NOME_ARQUIVO_GRUPO]);
void exibir_menu();
void *Cliente(void *args);
void *Servidor(void *args);

/*
 * Cliente TCP
 */
int main(argc, argv)
int argc;
char **argv;
{
	unsigned short port;                
	struct hostent *hostnm;    
	struct sockaddr_in server, client, client_mandar_mensagem;
	int namelen;
	int socket_cliente, novo_socket, socket_cliente_para_enviar_mensagem;
	int tc;
	int status_conectado_servidor; //Status de cadastro do cliente: 0 - nao cadastrado; 1 - cadastrado (online)
	int tamanho_string;
	char opcao;
	int clientes_online = 0;
	char opcao_adicionar_contato;
	char numero_telefone_recebido[TAM_NUMERO_TELEFONE], endereco_ip_recebido[TAM_ENDERECO_IP]; 
	int porta_recebida;
	char numero_telefone_adicionado[TAM_NUMERO_TELEFONE], endereco_ip_adicionado[TAM_ENDERECO_IP];
	int porta_adicionada;
	//Variaveis opcao 3
	int opcao_enviar_contato;
	char imprimir_contatos[TAM_NUMERO_TELEFONE];
	char telefone_para_enviar_mensagem[TAM_NUMERO_TELEFONE];
	char endereco_ip_para_enviar_mensagem[TAM_ENDERECO_IP];
	int porta_para_enviar_mensagem;
	char mensagem_para_enviar[TAM_MENSAGEM];
	//variaveis opcao 4
	int opcao_enviar_grupo;
	char imprimir_grupos[TAM_NOME_GRUPO];
	char nome_grupo_para_enviar_mensagem[TAM_NOME_GRUPO];
	char arquivo_grupo_para_enviar_mensagem[TAM_NOME_ARQUIVO_GRUPO];

	/*
	* O primeiro argumento (argv[1]) é o hostname do servidor.
	* O segundo argumento (argv[2]) é a porta do servidor.
	*/
	if(argc != 3)
	{
		fprintf(stderr, "Use: %s hostname porta\n", argv[0]);
		exit(1);
	}

	/*
	* Obtendo o endereço IP do servidor
	*/
	hostnm = gethostbyname(argv[1]);

	if(hostnm == (struct hostent *) 0)
	{
		fprintf(stderr, "Gethostbyname failed\n");
		exit(2);
	}

	port = (unsigned short) atoi(argv[2]);

	/*
	* Define o endereço IP e a porta do servidor
	*/
	server.sin_family      = AF_INET;
	server.sin_port        = htons(port);
	server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

	/*
	* Cria um socket TCP (stream) para aguardar conexões
	*/
	if((socket_cliente = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket()");
		exit(2);
	}

	/*
	* Define o endereço IP e a porta do cliente
	*/
	client.sin_family      = AF_INET;   /* Tipo do endereço             */
   	client.sin_port        = 0;         /* Escolhe uma porta disponível */
   	client.sin_addr.s_addr = INADDR_ANY;

	/*
	* Liga o cliente à porta definida anteriormente.
	*/
	if(bind(socket_cliente, (struct sockaddr *)&client, sizeof(client)) < 0)
	{
		perror("Bind()");
		exit(3);
	}

	namelen = sizeof(socket_cliente);

	if(getsockname(socket_cliente, (struct sockaddr *)&client, &namelen) < 0)
	{
		perror("Bind()");
		exit(1);
	}

	printf("Porta: %d\n", ntohs(client.sin_port));

	//Criando thread Cliente que aguardara por conexoes
	tc = pthread_create(&thread_cliente, NULL, Cliente, (void *) &socket_cliente);

	if(tc)
	{
		printf("ERRO: impossivel criar um thread cliente\n");
		exit(-1);
	}

	pthread_detach(thread_cliente);

	printf("Novo cadastro!\n");

	printf("Digite seu numero de telefone: ");
	__fpurge(stdin);
	fgets(numero_telefone, sizeof(numero_telefone), stdin);

	tamanho_string = strlen(numero_telefone);

	//retirando \n do final da string
	if(numero_telefone[tamanho_string-1] == '\n')
		numero_telefone[tamanho_string-1] = 0;

	if(mkdir(numero_telefone, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) 
	{

		perror("mkdir");
		exit(1);
	}

	sprintf(arquivo_contatos, "%s/%s_contatos.txt", numero_telefone, numero_telefone);
	sprintf(arquivo_nome_grupos, "%s/grupos.txt", numero_telefone);

	//Colocando porta e telefone em um buffer (sendbuf) e enviando ao servidor
	sprintf(sendbuf, "%d %s", ntohs(client.sin_port), numero_telefone);

	socket_servidor = estabelecer_conexao(server);

	if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) == -1)
	{
		perror("Send()");
		exit(5);
	}

	if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
	{
		perror("Recv()");
		exit(6);
	}

	do
	{
		printf("Menu:\n");
		printf("1 - Adicionar contato\n");
		printf("2 - Criar grupo\n");
		printf("3 - Enviar mensagem ou foto a um contato\n");
		printf("4 - Enviar mensagem ou foto a um grupo\n");
		printf("5 - Sair\n");
		printf("Selecione uma opcao: ");	
		__fpurge(stdin);		
		scanf("%c", &opcao);

		system("clear");

		switch(opcao)
		{
			case '1':	sprintf(sendbuf, "%d %s", ntohs(client.sin_port), numero_telefone);

					socket_servidor = estabelecer_conexao(server);

					if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) == -1)

					{
						perror("Send()");
						exit(5);
					}

					if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
					{
						perror("Recv()");
						exit(6);
					}

					if(strcmp(recvbuf, "Esperando requisicao") == 0)
					{
						strcpy(sendbuf, "Consultar");
				
						//Requisitando consulta ao servidor de clientes online
						if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
						{
							perror("Send()");
							exit(5);
						}

						strcpy(sendbuf, "OK");

						printf("* Clientes online no servidor: *\n");

						//Recebendo contatos online do servidor, um por um
						while(1)
						{
							if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
							{
								perror("Recv()");
								exit(6);
							}

							if(strcmp(recvbuf, "Terminou") == 0)
							{
								clientes_online = 0;
								break;
							}
								
							clientes_online++;

							sscanf(recvbuf, "%s", numero_telefone_recebido);

							printf("\t- Cliente (%d):\n", clientes_online);
							printf("\t\t|Numero de Telefone: %s|\n", numero_telefone_recebido);

							//Mandando confirmacao de recebimento
							if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
							{
								perror("Send()");
								exit(5);
							}
						}

						if(!(fp_contatos = fopen(arquivo_contatos, "a")))
						{
						    printf("Erro ao abrir o arquivo\n");
						    exit(1);
						}

						while(1)
						{
							printf("\nAdicionar algum destes contatos (s/n)? ");
							__fpurge(stdin);
							scanf("%c", &opcao_adicionar_contato);

							if(opcao_adicionar_contato == 's')
							{
								printf("\nNumero de Telefone para ser adicionado: ");
								__fpurge(stdin);
								fgets(numero_telefone_adicionado, sizeof(numero_telefone_adicionado), stdin);

								fprintf(fp_contatos, "%s", numero_telefone_adicionado);
							}
							else
							{
								break;
							}	
						}

						fclose(fp_contatos);
					}
					else
					{
						printf("Nao foi possivel enviar requisicao ao servidor\n");
					}

					system("clear");

					close(socket_servidor);

					break;


			case '2':	system("clear");

					printf("Lista de contatos do cliente:\n");

					if(!(fp_contatos = fopen(arquivo_contatos, "r")))
					{
					    printf("Erro ao abrir o arquivo\n");
					    exit(1);
					}

					while(1)
					{
						fscanf(fp_contatos, "%s", imprimir_contatos);

						if(!feof(fp_contatos))
						{
							printf("Telefone: %s\n", imprimir_contatos);
						}
						else
						{
							break;
						}				
					}

					fclose(fp_contatos);

					criar_grupo();

					system("clear");

					break;

			case '3':	system("clear");

					printf("Lista de contatos do cliente:\n");

					if(!(fp_contatos = fopen(arquivo_contatos, "r")))
					{
					    printf("Erro ao abrir o arquivo\n");
					    exit(1);
					}

					while(1)
					{
						fscanf(fp_contatos, "%s", imprimir_contatos);

						if(!feof(fp_contatos))
						{
							printf("Telefone: %s\n", imprimir_contatos);
						}
						else
						{
							break;
						}				
					}

					fclose(fp_contatos);

					printf("\nO que deseja mandar para um contato?\n");
					printf("1. Mandar mensagem\n");
					printf("2. Mandar foto\n");					
					printf("Opcao: ");
					scanf("%d", &opcao_enviar_contato);

					if(opcao_enviar_contato == 1)
					{
						sprintf(sendbuf, "%d %s", ntohs(client.sin_port), numero_telefone);

						socket_servidor = estabelecer_conexao(server);

						if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) == -1)

						{
							perror("Send()");
							exit(5);
						}

						if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
						{
							perror("Recv()");
							exit(6);
						}

						if(strcmp(recvbuf, "Esperando requisicao") == 0)
						{
							strcpy(sendbuf, "Porta e Endereco IP");

							if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
							{
								perror("Send()");
								exit(5);
							}

							if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
							{
								perror("Recv()");
								exit(6);
							}
							
							if(strcmp(recvbuf, "Cliente?") == 0)
							{
								printf("\nDigite o numero de telefone que deseja mandar a mensagem: ");
								__fpurge(stdin);
								fgets(telefone_para_enviar_mensagem, sizeof(telefone_para_enviar_mensagem), stdin);

								strcpy(sendbuf, telefone_para_enviar_mensagem);

								if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
								{
									perror("Send()");
									exit(5);
								}

								if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
								{
									perror("Recv()");
									exit(6);
								}

								if(strcmp(recvbuf, "Cliente nao esta online") == 0)
								{
									printf("%s\n", recvbuf);
								}
								else if(strcmp(recvbuf, "Nao ha usuarios online") == 0)
								{
									printf("%s\n", recvbuf);
								}
								else
								{
									printf("\nEndereco e porta de %s: %s\n", telefone_para_enviar_mensagem, recvbuf);

									sscanf(recvbuf, "%s %d", endereco_ip_para_enviar_mensagem, &porta_para_enviar_mensagem);

									client_mandar_mensagem.sin_family      = AF_INET;
									client_mandar_mensagem.sin_port        = htons(porta_para_enviar_mensagem);
									client_mandar_mensagem.sin_addr.s_addr = inet_addr(endereco_ip_para_enviar_mensagem);

									socket_cliente_para_enviar_mensagem = estabelecer_conexao(client_mandar_mensagem);

									sprintf(sendbuf, "%s", numero_telefone);

									if(send(socket_cliente_para_enviar_mensagem, sendbuf, strlen(sendbuf)+1, 0) < 0)
									{
										perror("Send()");
										exit(5);
									}

									if(recv(socket_cliente_para_enviar_mensagem, recvbuf, sizeof(recvbuf), 0) < 0)
									{
										perror("Recv()");
										exit(6);
									}

									//printf("\nMensagem recebida do cliente %s: %s\n", telefone_para_enviar_mensagem, recvbuf);

									printf("\nDigite a mensagem que deseja mandar: ");
									__fpurge(stdin);
									fgets(mensagem_para_enviar, sizeof(mensagem_para_enviar), stdin);

									sprintf(sendbuf, "%s\n", mensagem_para_enviar);

									if(send(socket_cliente_para_enviar_mensagem, sendbuf, strlen(sendbuf)+1, 0) < 0)
									{
										perror("Send()");
										exit(5);
									}

									if(recv(socket_cliente_para_enviar_mensagem, recvbuf, sizeof(recvbuf), 0) < 0)
									{
										perror("Recv()");
										exit(6);
									}

									//printf("\nMensagem recebida do cliente %s: %s\n", telefone_para_enviar_mensagem, recvbuf);

									close(socket_cliente_para_enviar_mensagem);
								}
							}
							else
							{
								printf("Servidor nao solicitou o numero do cliente\n");
							}	
						}
						else
						{
							printf("Nao foi possivel enviar requisicao ao servidor\n");
						}

						//system("clear");

						close(socket_servidor);
					}
					else if(opcao_enviar_contato = 2)
					{
						printf("Opcao nao implementada\n");

						close(socket_servidor);
					}
					else
					{
						printf("Opcao invalida!\n");
					}

					break;

			case '4':	system("clear");

					printf("Lista de grupos do cliente:\n");

					if(!(fp_nome_grupos = fopen(arquivo_nome_grupos, "r")))
					{
					    printf("Erro ao abrir o arquivo\n");
					    exit(1);
					}

					while(1)
					{
						fscanf(fp_nome_grupos, "%s", imprimir_grupos);

						if(!feof(fp_nome_grupos))
						{
							printf("Grupo: %s\n", imprimir_grupos);
						}
						else
						{
							break;
						}				
					}

					fclose(fp_nome_grupos);

					printf("\nO que deseja mandar para um grupo?\n");
					printf("1. Mandar mensagem\n");
					printf("2. Mandar foto\n");					
					printf("Opcao: ");
					scanf("%d", &opcao_enviar_grupo);

					if(opcao_enviar_grupo == 1)
					{
						sprintf(sendbuf, "%d %s", ntohs(client.sin_port), numero_telefone);

						socket_servidor = estabelecer_conexao(server);

						if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) == -1)

						{
							perror("Send()");
							exit(5);
						}

						if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
						{
							perror("Recv()");
							exit(6);
						}

						if(strcmp(recvbuf, "Esperando requisicao") == 0)
						{
							strcpy(sendbuf, "Portas e Enderecos IP");
				
							//
							if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
							{
								perror("Send()");
								exit(5);
							}

							if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
							{
								perror("Recv()");
								exit(6);
							}
							
							if(strcmp(recvbuf, "Clientes?") == 0)
							{
								printf("\nDigite o nome do grupo que deseja mandar a mensagem: ");
								__fpurge(stdin);
								fgets(nome_grupo_para_enviar_mensagem, sizeof(nome_grupo_para_enviar_mensagem), stdin);

								tamanho_string = strlen(nome_grupo_para_enviar_mensagem);

								//retirando \n do final da string
								if(nome_grupo_para_enviar_mensagem[tamanho_string-1] == '\n')
								{
									nome_grupo_para_enviar_mensagem[tamanho_string-1] = 0;
								}

								sprintf(arquivo_grupo_para_enviar_mensagem, "%s/grupo_%s.txt", numero_telefone, nome_grupo_para_enviar_mensagem);

								printf("Arquivo do grupo: %s\n", arquivo_grupo_para_enviar_mensagem);

								mandar_mensagem_para_grupo(arquivo_grupo_para_enviar_mensagem);

								close(socket_cliente_para_enviar_mensagem);
							}
							else
							{
								printf("Servidor nao solicitou o numero do cliente\n");
							}	
						}
						else
						{
							printf("Nao foi possivel enviar requisicao ao servidor\n");
						}

						//system("clear");

						close(socket_servidor);
					}
					else if(opcao_enviar_grupo = 2)
					{
						printf("Opcao nao implementada\n");

						close(socket_servidor);
					}
					else
					{
						printf("Opcao invalida!\n");
					}

					break;

			case '5':	sprintf(sendbuf, "%d %s", ntohs(client.sin_port), numero_telefone);

					socket_servidor = estabelecer_conexao(server);

					if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) == -1)

					{
						perror("Send()");
						exit(5);
					}

					if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
					{
						perror("Recv()");
						exit(6);
					}

					if(strcmp(recvbuf, "Esperando requisicao") == 0)
					{
						strcpy(sendbuf, "Sair");
				
						if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
						{
							perror("Send()");
							exit(5);
						}

						if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
						{
							perror("Recv()");
							exit(6);
						}

						printf("Resposta do servidor: %s\n", recvbuf); //Irei desconecta-lo

						strcpy(sendbuf, numero_telefone);

						if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
						{
							perror("Send()");
							exit(5);
						}

						if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
						{
							perror("Recv()");
							exit(6);
						}

						printf("Resposta do servidor: %s\n", recvbuf);
					}
					else
					{
						printf("Nao foi possivel enviar requisicao ao servidor\n");
					}

					//system("clear");

					close(socket_servidor);

					break;

			default:	printf("Selecione uma opcao valida!\n");
					break;
		}
	}while(opcao != '5');

	printf("Cliente terminou com sucesso.\n");
	exit(0);
}

int estabelecer_conexao(struct sockaddr_in server)
{
	int socket_criada;
 
	/*
	* Cria um socket TCP (stream)
	*/
	if((socket_criada = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket()");
		exit(3);
	}   

	/* Estabelece conexão com o servidor */
	if(connect(socket_criada, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Connect()");
		exit(4);
	}

	return socket_criada;
}

void criar_grupo()
{
	char arquivo_grupo[TAM_NOME_ARQUIVO_GRUPO];
	char nome_grupo[TAM_NOME_GRUPO];
	char telefone_grupo[TAM_NUMERO_TELEFONE];
	char opcao;
	int tamanho_string;
	FILE *fp_grupo;

	printf("\nDigite o nome do grupo que deseja criar: ");
	__fpurge(stdin);
	fgets(nome_grupo, sizeof(nome_grupo), stdin);

	tamanho_string = strlen(nome_grupo);

	//retirando \n do final da string
	if(nome_grupo[tamanho_string-1] == '\n')
	{
		nome_grupo[tamanho_string-1] = 0;
	}

	sprintf(arquivo_grupo, "%s/grupo_%s.txt", numero_telefone, nome_grupo);

	if(!(fp_nome_grupos = fopen(arquivo_nome_grupos, "a")))
	{
	    printf("Erro ao abrir o arquivo\n");
	    exit(1);
	}

	fprintf(fp_nome_grupos, "%s\n", nome_grupo);

	fclose(fp_nome_grupos);

	if(!(fp_grupo = fopen(arquivo_grupo, "a")))
	{
	    printf("Erro ao abrir o arquivo\n");
	    exit(1);
	}

	while(1)
	{
		printf("\nDigite o numero de telefone que deseja adicionar a esse grupo: ");
		__fpurge(stdin);
		fgets(telefone_grupo, sizeof(telefone_grupo), stdin);

		fprintf(fp_grupo, "%s", telefone_grupo);

		printf("\nDeseja adicionar mais um contato ao grupo (s/n)? ");
		__fpurge(stdin);
		scanf("%c", &opcao);

		if(opcao != 's')
		{
			break;
		}
	}

	fclose(fp_grupo);
}

void mandar_mensagem_para_grupo(char nome_grupo[TAM_NOME_ARQUIVO_GRUPO])
{
	struct sockaddr_in client_mandar_mensagem_grupo;
	int socket_enviar_grupo;	
	char contato_grupo_para_enviar_mensagem[TAM_NUMERO_TELEFONE];
	char endereco_ip_para_enviar_mensagem_grupo[TAM_ENDERECO_IP];
	char mensagem_para_enviar_grupo[TAM_MENSAGEM];
	int porta_para_enviar_mensagem_grupo;
	FILE *fp_mandar_mensagem_grupo;

	if(!(fp_mandar_mensagem_grupo = fopen(nome_grupo, "r")))
	{
	    printf("Erro ao abrir o arquivo\n");
	    exit(1);
	}

	printf("\nDigite a mensagem que deseja mandar: ");
	__fpurge(stdin);
	fgets(mensagem_para_enviar_grupo, sizeof(mensagem_para_enviar_grupo), stdin);

	while(1)
	{
		fscanf(fp_mandar_mensagem_grupo, "%s", contato_grupo_para_enviar_mensagem);

		if(!feof(fp_mandar_mensagem_grupo))
		{
			printf("Telefone: %s\n", contato_grupo_para_enviar_mensagem);

			strcpy(sendbuf, contato_grupo_para_enviar_mensagem);

			if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
			{
				perror("Send()");
				exit(5);
			}

			if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
			{
				perror("Recv()");
				exit(6);
			}

			printf("\nEndereco e porta de %s: %s\n", contato_grupo_para_enviar_mensagem, recvbuf);

			sscanf(recvbuf, "%s %d", endereco_ip_para_enviar_mensagem_grupo, &porta_para_enviar_mensagem_grupo);			

			client_mandar_mensagem_grupo.sin_family      = AF_INET;
			client_mandar_mensagem_grupo.sin_port        = htons(porta_para_enviar_mensagem_grupo);
			client_mandar_mensagem_grupo.sin_addr.s_addr = inet_addr(endereco_ip_para_enviar_mensagem_grupo);

			socket_enviar_grupo = estabelecer_conexao(client_mandar_mensagem_grupo);

			sprintf(sendbuf, "%s", numero_telefone);

			if(send(socket_enviar_grupo, sendbuf, strlen(sendbuf)+1, 0) < 0)
			{
				perror("Send()");
				exit(5);
			}

			if(recv(socket_enviar_grupo, recvbuf, sizeof(recvbuf), 0) < 0)
			{
				perror("Recv()");
				exit(6);
			}

			//printf("\nMensagem recebida do cliente %s: %s\n", telefone_para_enviar_mensagem, recvbuf);

			sprintf(sendbuf, "%s\n", mensagem_para_enviar_grupo);

			if(send(socket_enviar_grupo, sendbuf, strlen(sendbuf)+1, 0) < 0)
			{
				perror("Send()");
				exit(5);
			}

			if(recv(socket_enviar_grupo, recvbuf, sizeof(recvbuf), 0) < 0)
			{
				perror("Recv()");
				exit(6);
			}

			printf("Recvbuf: %s\n", recvbuf);
		}
		else
		{
			strcpy(sendbuf, "Todas as localizacoes ja foram solicitadas");

			if(send(socket_servidor, sendbuf, strlen(sendbuf)+1, 0) < 0)
			{
				perror("Send()");
				exit(5);
			}

			if(recv(socket_servidor, recvbuf, sizeof(recvbuf), 0) < 0)
			{
				perror("Recv()");
				exit(6);
			}

			break;
		}				
	}

	fclose(fp_mandar_mensagem_grupo);
}

void exibir_menu()
{
	printf("Menu:\n");
	printf("1 - Adicionar contato\n");
	printf("2 - Criar grupo\n");
	printf("3 - Enviar mensagem ou foto a um contato\n");
	printf("4 - Enviar mensagem ou foto a um grupo\n");
	printf("5 - Sair\n");
	printf("Selecione uma opcao:\n");
}

void *Cliente(void *args)
{
	int ts;

	struct sockaddr_in client;
	int socket_cliente, novo_socket;
	int namelen;

	socket_cliente = *(int *)args;

	if(listen(socket_cliente, NO_CLIENTES) != 0)
	{
		perror("Listen()");
		exit(4);
	}

	while(1)
	{
		namelen = sizeof(client);

		if((novo_socket = accept(socket_cliente, (struct sockaddr *)&client, &namelen)) == -1)
		{
			perror("Accept()");
			exit(5);
		}

		ts = pthread_create(&thread_servidor, NULL, Servidor, (void *) &novo_socket);

		if(ts)
		{
			printf("ERRO: impossivel criar um thread servidor\n");
			exit(-1);
		}

		pthread_detach(thread_servidor);
	}

	close(novo_socket);
	
	pthread_exit(NULL);
}

void *Servidor(void *args)
{
	struct sockaddr_in client;
	char sendbuf[TAM_SENDBUFF];              
	char recvbuf[TAM_RECVBUFF];
	int novo_socket;
	char arquivo_mensagens[TAM_NOME_ARQUIVO_MENSAGENS];
	FILE *fp_mensagem_recebida;

	novo_socket = *(int *)args;

	//printf("Thread: %d\n", novo_socket);

	system("clear");

	if(recv(novo_socket, recvbuf, sizeof(recvbuf), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	printf("Mensagem de %s:\n", recvbuf);

	sprintf(arquivo_mensagens, "%s/%s_mensagens.txt", numero_telefone, recvbuf);

	if(!(fp_mensagem_recebida = fopen(arquivo_mensagens, "a")))
	{
	    printf("Erro ao abrir o arquivo\n");
	    exit(1);
	}

	fprintf(fp_mensagem_recebida, "Mensagem de %s:\n", recvbuf);

	strcpy(sendbuf, "Esperando mensagem");

	if(send(novo_socket, sendbuf, strlen(sendbuf)+1, 0) < 0)
	{
		perror("Send()");
		exit(5);
	}

	if(recv(novo_socket, recvbuf, sizeof(recvbuf), 0) == -1)
	{
		perror("Recv()");
		exit(6);
	}

	printf("%s\n", recvbuf); //"Mensagem"

	fprintf(fp_mensagem_recebida, "%s", recvbuf);

	fclose(fp_mensagem_recebida);

	exibir_menu();

	strcpy(sendbuf, "Mensagem recebida");
			
	if(send(novo_socket, sendbuf, strlen(sendbuf)+1, 0) < 0)
	{
		perror("Send()");
		exit(5);
	}

	close(novo_socket);
	
	pthread_exit(NULL);	
}

