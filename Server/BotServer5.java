import java.io.*;
import java.net.*;
import java.sql.*;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class BotServer5
{
    private static final String HOST = "0.0.0.0";
    private static final int PORT = 22720;
    private static final int THREAD_POOL_SIZE = 10;

    private static final String DB_USER = "u256528_A7rq65PdJU";
	private static final String connStr = "jdbc:mysql://mysql.db.bot-hosting.net/s256528_USERS?requireSSL=false";;
	private static final String DB_PASSWORD ="RMWuNTZr6@FG.mUVMJccQjYI";


    private static final List<String> messages = new ArrayList<>();
    private static final String MESSAGE_FILE = "messages.txt";

    public static void main(String[] args)
	{
        // Forza il caricamento del driver JDBC
		try
		{

			URL jarUrl = new URL("file:mysql-connector-j-8.1.0.jar");
			URLClassLoader loader = new URLClassLoader(new URL[] { jarUrl });

			Driver driver = (Driver)
				Class.forName("com.mysql.jdbc.Driver", true, loader).getDeclaredConstructor().newInstance();

			DriverManager.registerDriver(driver);

			System.out.println("[INFO] Driver JDBC registrato correttamente.");
		}
		catch (Exception e)
		{
			System.err.println("[ERRORE] Impossibile caricare il driver JDBC: " + e.getMessage());
			e.printStackTrace();
		} 
		try (Connection conn =  DriverManager.getConnection(connStr, DB_USER, DB_PASSWORD)) {
			System.out.println("Connessione al database riuscita!");
		} catch (Exception e) {
			System.err.println("Errore durante la connessione: " + e.getMessage());
			e.printStackTrace();
		}

        ExecutorService threadPool = Executors.newFixedThreadPool(THREAD_POOL_SIZE);

        // Carica i messaggi salvati in locale
        loadMessages();

        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            System.out.println("[AVVIO] Server avviato su " + HOST + ":" + PORT);

            while (true)
			{
                Socket clientSocket = serverSocket.accept();
                System.out.println("[CONNESSIONE] Nuovo client connesso: " + clientSocket.getInetAddress());
                threadPool.execute(() -> handleClient(clientSocket));
            }
        } catch (IOException e) {
            System.err.println("[ERRORE] Errore durante l'avvio del server: " + e.getMessage());
        }
    }

    private static void handleClient(Socket clientSocket)
	{
        try (BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
		PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)) {

            out.println("Benvenuto nella BBS PC128\n");
            out.println("Comandi disponibili: 'login <username> <password>', 'register <username> <password>', 'exit'.");

            String message;
            boolean loggedIn = false;
            String username = null;

            while ((message = in.readLine()) != null)
			{
                if (!loggedIn)
				{
                    if (message.startsWith("register "))
					{
                        String[] parts = message.split(" ");
                        if (parts.length == 3 && registerUser(parts[1], parts[2]))
						{
                            out.println("Registrazione effettuata con successo! Ora puoi effettuare il login.");
                        }
						else
						{
                            out.println("Errore durante la registrazione. Riprova.");
                        }
                    }
					else if (message.startsWith("login "))
					{
                        String[] parts = message.split(" ");
                        if (parts.length == 3 && authenticate(parts[1], parts[2]))
						{
                            loggedIn = true;
                            username = parts[1];
                            out.println("Login effettuato con successo! Benvenuto " + username + ".");
                            out.println("Comandi disponibili: help, read, post, upload, download, exit.");
                        }
						else
						{
                            out.println("Login fallito. Riprova.");
                        }
                    }
					else
					{
                        out.println("Devi registrarti o effettuare il login prima di accedere ai comandi.");
                    }
                }
				else
				{
                    switch (message.toLowerCase().split(" ")[0])
					{
                        case "exit":
                            out.println("Disconnessione dal server. Arrivederci!");
                            return;

                        case "help":
                            out.println("Comandi disponibili: help, read, post, upload, download, exit.");
                            break;

                        case "read":
                            out.println("Messaggi in bacheca:");
                            synchronized (messages)
							{
                                for (String msg : messages)
								{
                                    out.println(msg);
                                }
                            }
                            break;

                        case "post":
                            out.println("Inserisci il tuo messaggio:");
                            String newMessage = in.readLine();
                            String timestampedMessage = "[" + getCurrentTime() + "] " + username + ": " + newMessage;
                            synchronized (messages)
							{
                                messages.add(timestampedMessage);
                            }
                            saveMessages();
                            out.println("Messaggio pubblicato nella bacheca!");
                            break;

                        case "upload":
                            uploadFile(message, clientSocket, in, out);
                            break;

                        case "download":
                            downloadFile(clientSocket);
                            break;
							
						case "dir":
							dir(out);
							break;

                        default:
                            out.println("Comando non riconosciuto. Digita 'help' per assistenza.");
                            break;
                    }
                }
            }
        } catch (IOException e) {
            System.err.println("[ERRORE] Problema con il client: " + e.getMessage());
        } finally {
            try
			{
                clientSocket.close();
                System.out.println("[DISCONNESSIONE] Client disconnesso.");
            }
			catch (IOException e)
			{
                System.err.println("[ERRORE] Errore durante la chiusura della connessione: " + e.getMessage());
            }
        }
    }

    private static boolean registerUser(String username, String password)
	{
        String salt = PasswordUtils.generateSalt();
        String hashedPassword = PasswordUtils.hashPassword(password, salt);

        try (Connection conn = DriverManager.getConnection(connStr, DB_USER, DB_PASSWORD);
		PreparedStatement stmt = conn.prepareStatement("INSERT INTO users (username, password, salt) VALUES (?, ?, ?)")) {
            stmt.setString(1, username);
            stmt.setString(2, hashedPassword);
            stmt.setString(3, salt);
            stmt.executeUpdate();
            return true;
        } catch (SQLException e) {
            System.err.println("[ERRORE] Impossibile registrare l'utente: " + e.getMessage());
            return false;
        }
    }

    private static boolean authenticate(String username, String password)
	{
        try (Connection conn = DriverManager.getConnection(connStr, DB_USER, DB_PASSWORD);
		PreparedStatement stmt = conn.prepareStatement("SELECT password, salt FROM users WHERE username = ?")) {
            stmt.setString(1, username);
            ResultSet rs = stmt.executeQuery();
            if (rs.next())
			{
                String storedHash = rs.getString("password");
                String salt = rs.getString("salt");
                return PasswordUtils.verifyPassword(password, salt, storedHash);
            }
        } catch (SQLException e) {
            System.err.println("[ERRORE] Impossibile autenticare l'utente: " + e.getMessage());
        }
        return false;
    }

    private static void loadMessages()
	{
        try (BufferedReader reader = new BufferedReader(new FileReader(MESSAGE_FILE))) {
            String line;
            while ((line = reader.readLine()) != null)
			{
                messages.add(line);
            }
        } catch (IOException e) {
            System.err.println("[ERRORE] Errore nel caricamento dei messaggi: " + e.getMessage());
        }
    }

    private static void saveMessages()
	{
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(MESSAGE_FILE))) {
            for (String message : messages)
			{
                writer.write(message);
                writer.newLine();
            }
        } catch (IOException e) {
            System.err.println("[ERRORE] Errore nel salvataggio dei messaggi: " + e.getMessage());
        }
    }

	private static void dir(PrintWriter out)
	{
		File folder = new File("uploads");
		File[] listOfFiles = folder.listFiles();
		if(listOfFiles != null) {
			for (int i = 0; i < listOfFiles.length; i++) {
				if (listOfFiles[i].isFile()) {
					out.println(listOfFiles[i].getName());
				} else if (listOfFiles[i].isDirectory()) {
					out.println("/" + listOfFiles[i].getName());
				}
			}
		}
	}
	private static void uploadFile(String message, Socket clientSocket , BufferedReader in, PrintWriter out)
	{

		String [] params=message.split(" ");
		if (params.length == 6)
		{

			try
			{
				String fileName=params[1];
				short start=Short.valueOf(params[2]);
				short size=Short.valueOf(params[3]);
				short exec=Short.valueOf(params[4]);
				byte bank=Byte.valueOf(params[5]);

				// Scrive i dati nel file
				InputStream inb = null;
				OutputStream outb = null;


				try
				{
					inb = clientSocket.getInputStream();
				}
				catch (IOException ex)
				{
					System.out.println("Can't get socket input stream. ");
				}

				try
				{
					outb = new FileOutputStream("uploads/" + fileName);
				}
				catch (FileNotFoundException ex)
				{
					System.out.println("File not found. ");
				}

				byte[] bytes = new byte[16 * 1024];
				bytes[0] = (byte)(start / 256);
				bytes[1] = (byte)(start % 256);

				bytes[2] = (byte)(size / 256);
				bytes[3] = (byte)(size % 256);

				bytes[4] = (byte)(exec / 256);
				bytes[5] = (byte)(exec % 256);
				bytes[6] = (byte)bank;

				outb.write(bytes, 0, 7);   
				int i=0;
				int count=0;
				while (i < size)
				{
					count = inb.read(bytes);
					outb.write(bytes, 0, count);
					i += count;
				}

				out.println("File caricato con successo.");

			}
			catch (IOException e)
			{

				e.printStackTrace();
			}
			try
			{
				clientSocket.setSoTimeout(0);
			}
			catch (IOException e)
			{}
		}
		else
			out.println("mancano parametri .");

	}





	private static void downloadFile(Socket clientSocket)
	{
		try
		{
			DataOutputStream dataOut = new DataOutputStream(clientSocket.getOutputStream());
			BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));

			dataOut.writeUTF("Inserisci il nome del file da scaricare:");
			dataOut.flush();
			String fileName = in.readLine();

			File file = new File("uploads/" + fileName);
			if (file.exists() && file.isFile())
			{
				dataOut.writeUTF("Inizio scaricamento del file: " + fileName);
				dataOut.writeLong(file.length());

				try (FileInputStream fileIn = new FileInputStream(file)) {
					byte[] buffer = new byte[4096];
					int bytesRead;
					while ((bytesRead = fileIn.read(buffer)) > 0)
					{
						dataOut.write(buffer, 0, bytesRead);
					}
				}

				dataOut.flush();
			}
			else
			{
				dataOut.writeUTF("File non trovato.");
			}
		}
		catch (IOException e)
		{
			try
			{
				DataOutputStream dataOut = new DataOutputStream(clientSocket.getOutputStream());
				dataOut.writeUTF("Errore durante il download del file: " + e.getMessage());
			}
			catch (IOException ex)
			{
				System.err.println("Errore durante l'invio del messaggio di errore: " + ex.getMessage());
			}
		}
	}
	private static String getCurrentTime()
	{
        return new java.util.Date().toString();
    }

}
