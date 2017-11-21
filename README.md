# Sistema de envio de mensagen peer-to-peer (WhatsAp2p)
------------------------------------------------------------------------------------------------------------------------------------------
Projeto desenvolvido para a disciplina de Redes de Computadores A
------------------------------------------------------------------------------------------------------------------------------------------
Utilizando a linguagem C, foi desenvolvido um sistema de troca de
mensagens peer-to-peer,
através da rede, utilizando o protocolo TCP. Esse
sistema é composto por módulos de usuários que se comunicam entre si, e com um
servidor central, ou seja, é um sistema semelhante ao antigo Napster. </br>
O servidor central é responsável pelo armazenamento dos usuários que estão
online, salvando informações dos clientes em uma lista ligada, e permitindo que
todos os clientes possam verificar essas informações. </br>
Os módulos de usuários comunicam-se através de conexões P2P (peer-to-peer),
ou seja, utilizam uma arquitetura de redes de computadores, na
qual, cada um dos nós da rede atua tanto como servidor como cliente, permitindo a
troca de mensagens sem a necessidade de um servidor central. Para essa
comunicação, os módulos de usuários verificam no servidor central os números de
telefone que estão online, recebendo o número da porta e o endereço IP do contato
desejado, enviando assim, a mensagem.
