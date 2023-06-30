# tcp-chat-room

## Projeto de Ambientes Virtuais de Conversação com 

System Call Select()

Este projeto consiste em uma aplicação de salas de bate-papo virtuais implementada em linguagem C, utilizando a system call select() para gerenciar as interações entre os clientes e o servidor. 

### Requisitos

- GCC (GNU Compiler Collection) instalado no sistema.

### Como Executar

1. Faça o download dos arquivos `client.c` e `server.c` clonando este repositório.

2. Compile o código fonte do servidor:
   
   ```shell
   gcc server.c -o server
   ```

3. Compile o código fonte do cliente:
   
   ```shell
   gcc client.c -o client
   ```

4. Execute o servidor:
   
   ```shell
   ./server
   ```

5. Em outro terminal, execute o cliente:
   
   ```shell
   ./client
   ```

Agora você pode interagir com o cliente e o servidor por meio dos comandos implementados no código-fonte. Também é possível se comunicar com o servidor por meio do Telnet.


