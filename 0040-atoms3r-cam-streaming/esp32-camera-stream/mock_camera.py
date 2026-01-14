#!/usr/bin/env python3
"""
Mock ESP32 M5Stack Camera Client
Generates animated JPEG frames and streams them via WebSocket
"""

import asyncio
import websockets
import io
import time
import math
from PIL import Image, ImageDraw, ImageFont
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class MockCamera:
    def __init__(self, width=640, height=480, fps=30):
        self.width = width
        self.height = height
        self.fps = fps
        self.frame_count = 0
        self.running = False
        
    def generate_frame(self):
        """Generate an animated JPEG frame"""
        # Create a new image
        img = Image.new('RGB', (self.width, self.height), color='black')
        draw = ImageDraw.Draw(img)
        
        # Animation parameters
        t = self.frame_count / self.fps
        
        # Draw animated gradient background
        for y in range(self.height):
            color_value = int(128 + 127 * math.sin(t * 2 + y / 50))
            color = (color_value // 3, color_value // 2, color_value)
            draw.line([(0, y), (self.width, y)], fill=color)
        
        # Draw moving circle
        circle_x = int(self.width / 2 + (self.width / 3) * math.cos(t * 2))
        circle_y = int(self.height / 2 + (self.height / 3) * math.sin(t * 3))
        circle_radius = int(40 + 20 * math.sin(t * 5))
        
        # Circle with gradient effect
        for r in range(circle_radius, 0, -2):
            intensity = int(255 * (r / circle_radius))
            color = (255, intensity, intensity // 2)
            draw.ellipse(
                [circle_x - r, circle_y - r, circle_x + r, circle_y + r],
                fill=color
            )
        
        # Draw moving square
        square_x = int(self.width / 2 + (self.width / 4) * math.sin(t * 1.5))
        square_y = int(self.height / 2 + (self.height / 4) * math.cos(t * 1.5))
        square_size = 30
        draw.rectangle(
            [square_x - square_size, square_y - square_size,
             square_x + square_size, square_y + square_size],
            outline=(100, 255, 100),
            width=3
        )
        
        # Draw frame counter and info
        try:
            # Try to use a default font, fallback to basic if not available
            font = ImageFont.load_default()
        except:
            font = None
        
        info_text = f"ESP32 M5Stack Mock Camera\nFrame: {self.frame_count}\nFPS: {self.fps}\nTime: {t:.2f}s"
        
        # Draw text background
        text_bbox = draw.textbbox((10, 10), info_text, font=font)
        draw.rectangle(text_bbox, fill=(0, 0, 0, 128))
        draw.text((10, 10), info_text, fill=(255, 255, 255), font=font)
        
        # Draw border
        draw.rectangle([0, 0, self.width-1, self.height-1], outline=(255, 255, 255), width=2)
        
        # Convert to JPEG bytes
        buffer = io.BytesIO()
        img.save(buffer, format='JPEG', quality=85)
        jpeg_data = buffer.getvalue()
        
        self.frame_count += 1
        return jpeg_data
    
    async def stream_to_server(self, server_url):
        """Stream frames to the server via WebSocket"""
        logger.info(f"Connecting to server at {server_url}...")
        
        retry_count = 0
        max_retries = 5
        
        while retry_count < max_retries:
            try:
                async with websockets.connect(server_url, max_size=10*1024*1024) as websocket:
                    logger.info("Connected to server!")
                    retry_count = 0  # Reset retry count on successful connection
                    self.running = True
                    
                    frame_interval = 1.0 / self.fps
                    
                    while self.running:
                        start_time = time.time()
                        
                        # Generate and send frame
                        jpeg_frame = self.generate_frame()
                        await websocket.send(jpeg_frame)
                        
                        if self.frame_count % 30 == 0:
                            logger.info(f"Sent frame {self.frame_count} ({len(jpeg_frame)} bytes)")
                        
                        # Maintain frame rate
                        elapsed = time.time() - start_time
                        sleep_time = max(0, frame_interval - elapsed)
                        await asyncio.sleep(sleep_time)
                        
            except websockets.exceptions.ConnectionClosed:
                logger.warning("Connection closed by server")
                break
            except ConnectionRefusedError:
                retry_count += 1
                logger.warning(f"Connection refused. Retry {retry_count}/{max_retries} in 2 seconds...")
                await asyncio.sleep(2)
            except Exception as e:
                logger.error(f"Error during streaming: {e}")
                retry_count += 1
                if retry_count < max_retries:
                    logger.info(f"Retrying in 2 seconds... ({retry_count}/{max_retries})")
                    await asyncio.sleep(2)
                else:
                    break
        
        if retry_count >= max_retries:
            logger.error("Max retries reached. Giving up.")
        
        self.running = False
    
    def stop(self):
        """Stop streaming"""
        logger.info("Stopping camera...")
        self.running = False

async def main():
    # Configuration
    SERVER_URL = "ws://localhost:8080/ws/camera"
    WIDTH = 640
    HEIGHT = 480
    FPS = 30
    
    logger.info("Mock ESP32 M5Stack Camera Client")
    logger.info(f"Resolution: {WIDTH}x{HEIGHT}")
    logger.info(f"Frame rate: {FPS} fps")
    logger.info(f"Server URL: {SERVER_URL}")
    
    camera = MockCamera(width=WIDTH, height=HEIGHT, fps=FPS)
    
    try:
        await camera.stream_to_server(SERVER_URL)
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
        camera.stop()

if __name__ == '__main__':
    asyncio.run(main())
