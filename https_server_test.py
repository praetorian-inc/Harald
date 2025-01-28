from aiohttp import web
import random
import ssl


test_text = """Never gonna give you up
Never gonna let you down
Never gonna run around and desert you
Never gonna make you cry
Never gonna say goodbye
Never gonna tell a lie and hurt you""".split('\n')

async def handle_post(request: web.Request):
    # Handle POST request data
    try:
        post_data = await request.text()  # Parse JSON data
        # Process the data asynchronously
        print(f"Received data...")
        print(post_data)
        
        # Return a response
        response = web.Response(text=random.choice(test_text), status=200)
        await response.prepare(request)
        await response.write_eof()

        return response

    except Exception as e:
        return web.json_response({"status": "error", "message": str(e)}, status=500)

async def init_app():
    # Create the aiohttp web application
    app = web.Application()
    
    # Add the route to handle POST requests
    app.router.add_post('/', handle_post)
    
    return app

def start_server():
    app = web.Application()

    # Initialize the application
    app = init_app()

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.load_cert_chain(certfile='./cert.pem', keyfile='./key.pem')

    # Start the aiohttp web server
    web.run_app(app, host="localhost", port=1234, ssl_context=ssl_context)

if __name__ == '__main__':
    start_server()

