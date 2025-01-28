import socket
import random


def handle_client_connection(client_socket):
	random_strings = [
	"There must be some way out of here", 
	"Said the joker to the thief", 
	"There's too much confusion", 
	"I can't get no relief", 
	"Businessmen, they drink my wine", 
	"Plowmen dig my earth", 
	"None of them along the line", 
	"Know what any of it is worth", 
	]
      
	try:
		data = client_socket.recv(2048)
		if data:
			print(f"Received: {data.decode('utf-8')}")
			
			random_string = random.choice(random_strings)
			client_socket.send(random_string.encode('utf-8'))
	except Exception as e:
		print(f"Error: {e}")
	finally:
		client_socket.close()

def start_server(host='0.0.0.0', port=5432):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    server_socket.bind((host, port))
    
    server_socket.listen(5)
    print(f"Server listening on {host}:{port}")
    
    try:
        while True:
            client_socket, addr = server_socket.accept()
            print(f"New connection from {addr}")
            
            handle_client_connection(client_socket)
    except KeyboardInterrupt:
        print("\nShutting down the server.")
    finally:
        server_socket.close()

# Start the server
if __name__ == "__main__":
    start_server()