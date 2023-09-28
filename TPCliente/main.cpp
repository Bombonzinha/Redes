#include <iostream>
#include <winsock2.h>

void menuAdmin();
void menuConsulta();

int main() {
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        std::cerr << "Error al inicializar Winsock" << std::endl;
        return -1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket del cliente" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5005);
    serverAddr.sin_addr.s_addr = inet_addr("192.168.1.34"); // Cambiar a la dirección del servidor

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error al conectar al servidor" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Conectado al servidor! Ingrese Usuario|Contrasena" << std::endl;

    char buffer[1024];
    std::string rolUsuario;
    bool menuMode = false;
    while (true) {
        std::cout << "Cliente: ";
        std::cin.getline(buffer, sizeof(buffer));
        std::string numero = buffer; // Esto es para guardar el numero que eligió el usuario
        send(clientSocket, buffer, strlen(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Error al recibir datos del servidor" << std::endl;
            break;
        } else if (bytesReceived == 0) {
            std::cout << "Servidor desconectado" << std::endl;
            break;
        }
        std::cout << "Servidor: " << buffer << std::endl;
        // Verificar el rol del usuario y mostrar el menú adecuado
        // Los primeros 2 if es cuando hay que iniciar sesión
//        if (strcmp(buffer, "Acceso concedido como CONSULTA") == 0 && !menuMode) {
//            rolUsuario = "CONSULTA";
//            menuMode = true;
//        } else if (strcmp(buffer, "Acceso concedido como ADMIN") == 0 && !menuMode) {
//            rolUsuario = "ADMIN";
//            menuMode = true;
//        } else if (buffer[0] == '$'){ // Esto es cuando se muestra el registro, el primer caracter es "%" entonces menu=true
//            std::cout << "\nPresiona enter para cargar el resto del registro..." << std::endl;
//            menuMode = false;
//        } else if (buffer[0] == '%'){
//            std::cout << "\nYa se mostró todo el registo" << std::endl;
//            menuMode = true;
//        } else if (numero == "a" || "b"){
//            menuMode = false; // Acá lo pongo en false para que no me muestre el menú cuando elijo la opción 3
//        }
//        std::cout << menuMode << std::endl;
//        if (menuMode){ // Cuando NO hay que iniciar sesión
//            if (rolUsuario=="CONSULTA"){
//                menuConsulta();
//                menuMode = false;
//            } else if (rolUsuario=="ADMIN"){
//                menuAdmin();
//                menuMode = false;
//            }
//        } else {
//            menuMode = true;
//        }
//        std::cout << menuMode << std::endl;
    }


    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

void menuConsulta() {
    std::cout << "Servidor: " << std::endl;
    std::cout << "Menu de Consulta:" << std::endl;
    std::cout << "1. Traducir" << std::endl;
    std::cout << "5. Cerrar Sesion" << std::endl;
}

void menuAdmin() {
    std::cout << "Servidor: " << std::endl;
    std::cout << "Menu de Administrador:" << std::endl;
    std::cout << "1. Traducir" << std::endl;
    std::cout << "2. Nueva Traduccion" << std::endl;
    std::cout << "3. Usuarios:" << std::endl;
    std::cout << "   a. Alta" << std::endl;
    std::cout << "   b. Desbloqueo" << std::endl;
    std::cout << "4. Ver Registro de Actividades" << std::endl;
    std::cout << "5. Cerrar Sesion" << std::endl;
}

