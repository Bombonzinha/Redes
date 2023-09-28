#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <winsock2.h>

bool recibirDatos(SOCKET clientSocket, char* buffer, int bufferSize);

std::string leerArchivo(const std::string& nombreArchivo);

std::pair<bool, std::string> verificarCredenciales(const std::string& usuario, const std::string& contrasena);

void registrarLog(const std::string& mensaje);

std::string fechaYHora();

void traduccion(SOCKET clientSocket);

std::string buscarTraduccion(const std::string& palabra);

void nuevaTraduccion(SOCKET clientSocket);  // Esta es para PEDIR al cliente una nueva traducción

bool agregarNuevaTraduccion(const std::string& nuevaTraduccion); // Esta es para METER EN EL ARCHIVO la nueva traducción

void verRegistroActividades(SOCKET clientSocket); // Manda por varios buffer el registro completo

void administrarUsuarios(SOCKET clientSocket);
// Agrega un usuario al archivo credenciales.txt
bool registrarNuevoUsuario(const std::string& usuario, const std::string& contrasena, const std::string& rol, int intentos);

bool usuarioExiste(const std::string& usuario); // Función para verificar si un usuario ya está registrado

std::string obtenerUsuariosBloqueados(); // Devuelve un string con los usuarios que estan bloqueados

bool desbloquearUsuario(const std::string& usuario); // le asigna 0 intentos a un usuario con mas de 3

int main() {
    std::string mensajeLog="";
    std::string response;
    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        std::cerr << "Error al inicializar Winsock" << std::endl;
        return -1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket del servidor" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5005);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int reuseAddr = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr)) == SOCKET_ERROR) {
        std::cerr << "Error al establecer SO_REUSEADDR" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error al vincular el socket a la direccion" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    // Obtener el puerto de escucha
    sockaddr_in boundAddr;
    int boundAddrLen = sizeof(boundAddr);
    if (getsockname(serverSocket, (sockaddr*)&boundAddr, &boundAddrLen) == SOCKET_ERROR) {
        std::cerr << "Error al obtener el puerto de escucha" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    unsigned short puertoDeEscucha = ntohs(boundAddr.sin_port);

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Error al escuchar por conexiones entrantes" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    mensajeLog = "=============================\n =======Inicia Servidor=======\n=============================";
    registrarLog(mensajeLog);

    std::cout << "Esperando conexiones entrantes..." << std::endl;
    // Acá guardo el puerto de escucha
    mensajeLog = fechaYHora() + ": Socket creado. Puerto de escucha: " + std::to_string(puertoDeEscucha);
    registrarLog(mensajeLog);

    while (true) { // Bucle externo para esperar nuevas conexiones
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error al aceptar la conexion entrante" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return -1;
        }

        std::cout << "Cliente conectado" << std::endl;

        bool credencialesPedidas = false; // Creo el boolean para que las credenciales no esten en el while
        std::string usuario;
        std::string rolUsuario;
        char buffer[1024];
        while (true) {

            if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
                break; // Manejar error o desconexión del cliente
            }

            std::cout << "Cliente: " << buffer << std::endl;
            std::string mensajeCliente(buffer);

            if (!credencialesPedidas) { ///ESTE IF ES PARA LA PRIMERA VEZ, O SEA PARA INICIAR SESION, SU ELSE ES PARA ELEGIR OPCIONES
            // Parsear el mensaje para obtener usuario y contraseña
                size_t pos = mensajeCliente.find('|');
                if (pos == std::string::npos) {
                    std::cerr << "Mensaje invalido del cliente" << std::endl;
                    continue;
                }
                usuario = mensajeCliente.substr(0, pos);
                std::string contrasena = mensajeCliente.substr(pos + 1);

                // Verificar las credenciales

                std::pair<bool, std::string> credenciales = verificarCredenciales(usuario, contrasena);
                rolUsuario = credenciales.second; // Esto es para filtrar las opciones según el rol

                if (credenciales.first) {
                    response = "Acceso concedido como " + credenciales.second;
                    credencialesPedidas = true;
                    mensajeLog = fechaYHora() + ": Inicio de sesion - usuario: " + usuario;  // Esto va a registro.log
                    registrarLog(mensajeLog);
                } else {
                    response = "Acceso denegado";
                    break;
                }
                send(clientSocket, response.c_str(), response.size(), 0);
            } else { ///ACA SE ELIGEN OPCIONES Y SE HACEN LAS FUNCIONES NECESARIAS
                if (mensajeCliente == "1") { // Traducir palabra que recibe el servidor
                    traduccion(clientSocket);
                } else if (mensajeCliente == "2" && rolUsuario == "ADMIN") { // Agregar una traducción
                    nuevaTraduccion(clientSocket);
                } else if (mensajeCliente == "3" && rolUsuario == "ADMIN") { // Usuarios
                    administrarUsuarios(clientSocket);
                } else if (mensajeCliente == "4" && rolUsuario == "ADMIN") { // Ver registro de actividades
                    verRegistroActividades(clientSocket);
                } else if (mensajeCliente == "5") { // Cerrar Sesión
                    break; // Aqui cierra sesión
                } else { // Opción no válida
                    send(clientSocket, "Opcion no valida", strlen("Opcion no valida"), 0);
                }
                //response = "Mensaje recibido por el servidor";
            }
            //send(clientSocket, response.c_str(), response.size(), 0);
        }
        mensajeLog = fechaYHora() + ": Cierre de sesion - usuario: " + usuario;
        registrarLog(mensajeLog);
        closesocket(clientSocket); // Acá cierro el socket dentro del while, asi puedo...

        std::cout << "Esperando nuevas conexiones..." << std::endl; // Volver a esperar nuevas conexiones
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}

bool recibirDatos(SOCKET clientSocket, char* buffer, int bufferSize) {
    memset(buffer, 0, bufferSize);
    int bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error al recibir datos del cliente" << std::endl;
        return false;
    } else if (bytesReceived == 0) {
        std::cout << "Cliente desconectado" << std::endl;
        return false;
    }
    return true;
}

std::string leerArchivo(const std::string& nombreArchivo) { // Le mando por parámetro el nombre del archivo
    std::ifstream archivo(nombreArchivo); // Leer archivo
    std::string contenido; // String para cargar los datos (ACA VAN A ESTAR LOS STRINGS PARA COMPARAR)

    if (archivo.is_open()) { // Si el archivo está abierto
        std::string linea; // Línea temporal
        while (std::getline(archivo, linea)) { // Acá igualo la línea temporal a la siguiente línea del archivo (SE REEMPLAZA)
            contenido += linea + "\n"; // Le meto al string para cargar datos la línea temporal
        }
        archivo.close();
    } else {
        std::cerr << "Error al abrir el archivo" << std::endl;
    }

    return contenido;
}

std::pair<bool, std::string> verificarCredenciales(const std::string& usuario, const std::string& contrasena) {
    std::string contenidoArchivo = leerArchivo("credenciales.txt");

    std::istringstream archivoStream(contenidoArchivo); // Acá guardo las credenciales para estructurar funciones
    std::string linea;
    std::vector<std::string> lineas;

    bool usuarioBloqueado = false;
    bool usuarioEncontrado = false;
    bool accesoConcedido = false;

    std::string rolArchivoReturn = "";// Declaro rol para que me lo tome el return

    while (std::getline(archivoStream, linea)) { // Llamo una línea de las credenciales
        std::istringstream lineaStream(linea); // Lunciones para la línea
        std::string usuarioArchivo, contrasenaArchivo, rolArchivo, intentosArchivo; // Strings usuario|contraseña|rol|intentos
        std::getline(lineaStream, usuarioArchivo, '|');
        std::getline(lineaStream, contrasenaArchivo, '|');
        std::getline(lineaStream, rolArchivo, '|');
        std::getline(lineaStream, intentosArchivo);
        if (usuarioArchivo == usuario) {
            usuarioEncontrado = true;
            int intentos = std::stoi(intentosArchivo); // Convertir el valor a entero

            if (intentos >= 3) {
                usuarioBloqueado = true;
            } else if (contrasenaArchivo == contrasena) {
                intentos = 0;
                accesoConcedido = true;
                rolArchivoReturn = rolArchivo;
            } else {
                intentos++;
            }

            linea = usuarioArchivo + "|" + contrasenaArchivo + "|" + rolArchivo + "|" + std::to_string(intentos); // Paso a int intentos
        }

        lineas.push_back(linea); // Agregar la línea a la lista
    }

    if (!usuarioEncontrado) {
        std::cout << "Usuario no encontrado" << std::endl;
    }

    if (!usuarioBloqueado && !accesoConcedido) {
        std::ofstream archivo("credenciales.txt"); // Escribimos en el archivo
        for (const std::string& lineaModificada : lineas) { // Recorro cada línea modificada
            archivo << lineaModificada << "\n"; // La imprimo en el archivo
        }
    }
    // Devolver un par (bool, string) donde el bool indica si las credenciales son válidas
    // y el string contiene el rol del usuario
    return std::make_pair(usuarioEncontrado && accesoConcedido, rolArchivoReturn);
}

void registrarLog(const std::string& mensaje) {
    std::ofstream archivoLog("registro.log", std::ios::app); // Abre el archivo en modo de adjuntar
    if (archivoLog.is_open()) {
        archivoLog << mensaje << std::endl; // Escribe el mensaje en el archivo log
        archivoLog.close(); // Cierra el archivo
    } else {
        std::cerr << "Error al abrir el archivo de registro.log" << std::endl;
    }
}

std::string fechaYHora(){
    // Obtener la fecha y la hora actual del sistema
    std::time_t tiempoActual = std::time(nullptr);
    std::tm* tiempoInfo = std::localtime(&tiempoActual);

    // Formatear la fecha y la hora como una cadena
    std::stringstream ss;
    ss << std::put_time(tiempoInfo, "%Y-%m-%d_%H:%M");
    std::string fechaYHora = ss.str();

    return fechaYHora;
}

void traduccion(SOCKET clientSocket) {
    // Puedes recibir datos adicionales del cliente y enviar respuestas aquí
    char buffer[1024];
    // Pedir la palabra del cliente
    send(clientSocket, "Ingrese una palabra en ingles ", strlen("Ingrese una palabra en ingles "), 0);

    // Recibe la palabra en inglés del cliente
    if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
        return;
    }
    // Ahora con la palabra en mano, la mando en la funcion de buscar traduccion
    std::string palabra=buffer;
    if (palabra == "/salir") return;

    // Convertir la palabra a minúsculas
    std::transform(palabra.begin(), palabra.end(), palabra.begin(), ::tolower);


    // La traducción no existe en el archivo
    std::string resultado = buscarTraduccion(palabra);
    if (resultado.empty()) {
        send(clientSocket, ("No fue posible encontrar la traducción para: " + palabra).c_str(), strlen(("No fue posible encontrar la traducción para: " + palabra).c_str()), 0);
        return;
    }

    // Realiza la traducción y envía la respuesta al cliente
    send(clientSocket, resultado.c_str(), resultado.size(), 0);
}

std::string buscarTraduccion(const std::string& palabra) {
    const std::string archivoTraducciones = "traducciones.txt";

    std::ifstream archivo(archivoTraducciones); // Abro el archivo en modo lectura

    if (!archivo.is_open()) {
        return "Error: No se pudo abrir el archivo";
    }

    std::string linea;
    while (std::getline(archivo, linea)) {
        std::istringstream ss(linea);
        std::string palabraEnIngles, traduccion;

        if (std::getline(ss, palabraEnIngles, ':') && std::getline(ss, traduccion)) {
            // Eliminar espacios en blanco al principio y al final de las cadenas
            palabraEnIngles.erase(0, palabraEnIngles.find_first_not_of(" \t\r\n"));
            palabraEnIngles.erase(palabraEnIngles.find_last_not_of(" \t\r\n") + 1);

            traduccion.erase(0, traduccion.find_first_not_of(" \t\r\n"));
            traduccion.erase(traduccion.find_last_not_of(" \t\r\n") + 1);

            // Comparo con la palabra qwue mandé por parametro
            if (palabraEnIngles == palabra) {
                return palabra + " en ingles es " + traduccion + " en español.";
            }
        }
    }

    return "No se encontro traduccion para la palabra " + palabra;
}

void nuevaTraduccion(SOCKET clientSocket) {
    char buffer[1024];
    // Pedir la nueva traducción al cliente
    send(clientSocket, "Nueva traduccion palabraEnIngles:traduccionEnEspanol: ", strlen("Nueva traduccion palabraEnIngles:traduccionEnEspanol: "), 0);

    if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
        return;
    }

    std::string nuevaTraduccion=buffer; // Ejemplo: dog:perro
    if (nuevaTraduccion == "/salir") return;

    // Verificar el formato de inserción
    size_t pos = nuevaTraduccion.find(':');
    if (pos == std::string::npos || pos == 0 || pos == nuevaTraduccion.length() - 1) {
        send(clientSocket, "No fue posible insertar la traducción. El formato de inserción debe ser palabraEnInglés:traducciónEnEspañol", strlen("No fue posible insertar la traducción. El formato de inserción debe ser palabraEnInglés:traducciónEnEspañol"), 0);
        return;
    }

    std::string palabraEnIngles = nuevaTraduccion.substr(0, pos);
    std::string traduccionEnEspanol = nuevaTraduccion.substr(pos + 1); // Separo las palabras para buscarlas en el archivo

    // Verificar si la traducción ya existe
    if (buscarTraduccion(palabraEnIngles) == traduccionEnEspanol) {
        send(clientSocket, ("Ya existe una traducción para " + palabraEnIngles + ": " + traduccionEnEspanol).c_str(), ("Ya existe una traducción para " + palabraEnIngles + ": " + traduccionEnEspanol).size(), 0);
        return;
    }
    if (agregarNuevaTraduccion(nuevaTraduccion)) {
        send(clientSocket, "Nueva traduccion insertada correctamente", strlen("Nueva traduccion insertada correctamente"), 0);
    } else {
        send(clientSocket, "Error al insertar la nueva traduccion", strlen("Error al insertar la nueva traduccion"), 0);
    }
}

bool agregarNuevaTraduccion(const std::string& nuevaTraduccion) {
    std::ofstream archivoTraducciones("traducciones.txt", std::ios::app);

    if (!archivoTraducciones) {
        return false;
    }

    archivoTraducciones << nuevaTraduccion << std::endl;

    archivoTraducciones.close();

    return true;
}

void verRegistroActividades(SOCKET clientSocket) {
    std::ifstream archivoRegistro("registro.log");
    if (!archivoRegistro) {
        send(clientSocket, "ERROR: no hay archivo de registro", strlen("ERROR: no hay archivo de registro"), 0);
        return;
    }

    std::string linea;
    std::string contenidoRegistro;
    while (std::getline(archivoRegistro, linea)) { // Acá voy agregando al string final cada linea que lee el getline
        contenidoRegistro += linea + '\n';
    }
    archivoRegistro.close();

    std::string tempContenidoRegistro;
    while (contenidoRegistro.size()>=1023){ // Mientras el string final sea mayor al tamaño del buffer, le resto los primeros 1024 char
        tempContenidoRegistro = '$' + contenidoRegistro.substr(0, 1023); // Le agrego el $ para decirle al cliente que aún hay que mostrar más registro
        contenidoRegistro = contenidoRegistro.substr(1023);
        send(clientSocket, tempContenidoRegistro.c_str(), tempContenidoRegistro.size(), 0);
    }
    contenidoRegistro = '%' + contenidoRegistro; // Cuando se termina de mostrar todo el registro que hay en le buffer, le aviso al cliente con un % en el primer caracter
    send(clientSocket, contenidoRegistro.c_str(), contenidoRegistro.size(), 0);
}

void administrarUsuarios(SOCKET clientSocket) {
    char buffer[1024];
    send(clientSocket, "\nUsuarios:\na. Alta\nb. Desbloqueo\n", strlen("\nUsuarios:\na. Alta\nb. Desbloqueo\n"), 0);

    if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
        return; // Manejar error o desconexión del cliente
    }
    std::string opcion(buffer); // "a" o "b"
    if (opcion == "/salir") return;

    if (opcion == "a") { // Alta
        send(clientSocket, "Ingrese el nombre de usuario y contraseña separados por '|' (Ejemplo: usuario|contraseña): ", strlen("Ingrese el nombre de usuario y contraseña separados por '|' (Ejemplo: usuario|contraseña): "), 0);

        if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
            return; // Manejar error o desconexión del cliente
        }

        std::string datosUsuario(buffer);
        if (datosUsuario == "/salir") return;
        size_t pos = datosUsuario.find('|');
        if (pos == std::string::npos) {
            send(clientSocket, "Error al dar de alta el nuevo usuario: datos incompletos", strlen("Error al dar de alta el nuevo usuario: datos incompletos"), 0);
            return;
        }

        std::string nuevoUsuario = datosUsuario.substr(0, pos);
        std::string nuevaContrasena = datosUsuario.substr(pos + 1);

        // Validaciones adicionales, como verificar si el usuario ya existe y si los campos no están vacíos, deberían realizarse aquí
        if (!usuarioExiste(nuevoUsuario)){
            // Registrar el nuevo usuario, establecer intentos a 0, rol a CONSULTA
            registrarNuevoUsuario(nuevoUsuario, nuevaContrasena, "CONSULTA", 0);

            send(clientSocket, nuevoUsuario.c_str(), nuevoUsuario.size(), 0);
        } else {
            std::string mensajeError = "Error: El usuario '" + nuevoUsuario + "' ya existe.";
            send(clientSocket, mensajeError.c_str(), mensajeError.size(), 0);
        }
    } else if (opcion == "b") { // Desbloqueo de Usuario
        // Aquí debes implementar la lógica para mostrar los usuarios bloqueados y permitir al cliente seleccionar uno para desbloquear
        std::string usuariosBloqueados = obtenerUsuariosBloqueados();
        if (usuariosBloqueados.empty()) {
            send(clientSocket, "No se encontraron usuarios bloqueados", strlen("No se encontraron usuarios bloqueados"), 0);
            return;
        }

        send(clientSocket, usuariosBloqueados.c_str(), usuariosBloqueados.size(), 0);

        send(clientSocket, "Ingrese el nombre de usuario a desbloquear: ", strlen("Ingrese el nombre de usuario a desbloquear: "), 0);

        if (!recibirDatos(clientSocket, buffer, sizeof(buffer))) {
            return; // Manejar error o desconexión del cliente
        }

        std::string usuarioADesbloquear(buffer);
        if (usuarioADesbloquear == "/salir") return;

        // Realizar la lógica de desbloqueo del usuario
        bool desbloqueado = desbloquearUsuario(usuarioADesbloquear);

        if (desbloqueado) {
            send(clientSocket, (usuarioADesbloquear + " desbloqueado correctamente").c_str(), strlen((usuarioADesbloquear + " desbloqueado correctamente").c_str()), 0);
        } else {
            send(clientSocket, "Error al desbloquear el usuario", strlen("Error al desbloquear el usuario"), 0);
        }
    } else {
        send(clientSocket, "Opción no válida", strlen("Opción no válida"), 0);
    }
}

// Función para registrar un nuevo usuario en el archivo credenciales.txt
bool registrarNuevoUsuario(const std::string& usuario, const std::string& contrasena, const std::string& rol, int intentos) {
    // Abre el archivo en modo de escritura (appended)
    std::ofstream archivoCredenciales("credenciales.txt", std::ios::app);
    if (!archivoCredenciales) {
        return false; // No se pudo abrir el archivo
    }

    // Escribe el nuevo usuario en el archivo con el formato deseado
    archivoCredenciales << usuario << "|" << contrasena << "|" << rol << "|" << intentos << std::endl;

    // Cierra el archivo
    archivoCredenciales.close();

    return true; // El usuario se registró correctamente
}

// Función para verificar si un usuario ya existe en el archivo credenciales.txt
bool usuarioExiste(const std::string& usuario) {
    // Abre el archivo en modo de lectura
    std::ifstream archivoCredenciales("credenciales.txt");
    if (!archivoCredenciales) {
        return false; // No se pudo abrir el archivo
    }

    std::string linea;
    while (std::getline(archivoCredenciales, linea)) {
        size_t pos = linea.find('|');
        if (pos != std::string::npos) {
            std::string usuarioExistente = linea.substr(0, pos);
            if (usuarioExistente == usuario) {
                archivoCredenciales.close();
                return true; // El usuario ya existe en el archivo
            }
        }
    }

    archivoCredenciales.close();
    return false; // El usuario no fue encontrado en el archivo
}

std::string obtenerUsuariosBloqueados() {
    std::ifstream archivoCredenciales("credenciales.txt");
    if (!archivoCredenciales) {
        return "Error: No se pudo abrir el archivo de credenciales";
    }

    std::string usuariosBloqueados;
    std::string linea;
    while (std::getline(archivoCredenciales, linea)) {
        size_t pos = linea.find('|');
        if (pos != std::string::npos) {
            std::string usuario = linea.substr(0, pos);
            int intentos = std::stoi(linea.substr(linea.find_last_of('|') + 1)); // Obtener intentos como entero

            // Si el usuario tiene 3 intentos o más, considerarlo bloqueado
            if (intentos >= 3) {
                if (!usuariosBloqueados.empty()) {
                    usuariosBloqueados += ", ";
                }
                usuariosBloqueados += usuario;
            }
        }
    }

    archivoCredenciales.close();

    if (usuariosBloqueados.empty()) {
        return "No se encontraron usuarios bloqueados";
    }

    return usuariosBloqueados;
}

bool desbloquearUsuario(const std::string& usuario) {
    std::ifstream archivoEntrada("credenciales.txt");
    if (!archivoEntrada) {
        return false; // No se pudo abrir el archivo
    }

    std::vector<std::string> lineas;
    std::string linea;
    while (std::getline(archivoEntrada, linea)) {
        size_t pos = linea.find('|');
        if (pos != std::string::npos) {
            std::string nombreUsuario = linea.substr(0, pos);

            if (nombreUsuario == usuario) {
                // Encontramos el usuario, ahora lo modificamos para establecer intentos a 0
                size_t intentos = linea.find_last_of('|');
                if (intentos != std::string::npos) {
                    // Encuentra la posición del último pipe '|'
                    linea.replace(intentos + 1, std::string::npos, "0");
                }
            }
        }
        lineas.push_back(linea);
    }

    archivoEntrada.close();

    // Volver a escribir todas las líneas en el archivo
    std::ofstream archivoSalida("credenciales.txt");
    if (!archivoSalida) {
        return false; // No se pudo abrir el archivo
    }

    for (const std::string& linea : lineas) {
        archivoSalida << linea << '\n';
    }

    archivoSalida.close();

    return true;
}




