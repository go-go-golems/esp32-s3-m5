#!/usr/bin/env python3
import socket
import time

def send_and_receive(sock, cmd, wait_time=1.0):
    """Send a command and receive response"""
    print(f"\n>>> {cmd}")
    sock.sendall((cmd + '\r\n').encode())
    time.sleep(wait_time)
    
    # Read response
    sock.settimeout(0.5)
    response = b''
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
    except socket.timeout:
        pass
    
    output = response.decode('utf-8', errors='ignore')
    if output.strip():
        print(output)
    return output

def main():
    print("=" * 60)
    print("ESP32-S3 MicroQuickJS REPL - Storage Test")
    print("=" * 60)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print("\nConnecting to QEMU on localhost:5555...")
        sock.connect(('localhost', 5555))
        print("Connected!")
        
        # Wait for initial output
        time.sleep(2)
        sock.settimeout(0.5)
        try:
            initial = sock.recv(8192)
            print("\n" + "=" * 60)
            print("Initial Output:")
            print("=" * 60)
            print(initial.decode('utf-8', errors='ignore'))
        except socket.timeout:
            pass
        
        print("\n" + "=" * 60)
        print("Testing JavaScript Execution")
        print("=" * 60)
        
        # Test basic JavaScript
        tests = [
            ("1+2+3", "Basic arithmetic"),
            ("var x = 10", "Variable declaration"),
            ("x * 5", "Variable usage"),
            
            # Test if we can manually create the utilities
            ("var Math2 = {}", "Create object"),
            ("Math2.square = function(x) { return x * x; }", "Add method"),
            ("Math2.square(5)", "Call method"),
            
            # Test factorial function
            ("var fact = function(n) { if (n <= 1) return 1; return n * fact(n-1); }", "Define factorial"),
            ("fact(5)", "Calculate 5!"),
            ("fact(10)", "Calculate 10!"),
            
            # Test string operations
            ("var str = 'Hello'", "String variable"),
            ("str.length", "String length"),
            
            # Test arrays
            ("[1,2,3,4,5]", "Array literal"),
            ("var arr = [10, 20, 30]", "Array variable"),
            ("arr[1]", "Array access"),
            ("arr.length", "Array length"),
        ]
        
        for cmd, desc in tests:
            print(f"\n--- Test: {desc} ---")
            send_and_receive(sock, cmd, 0.8)
        
        print("\n" + "=" * 60)
        print("All tests completed!")
        print("=" * 60)
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        sock.close()

if __name__ == '__main__':
    main()
