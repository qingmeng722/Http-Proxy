# Use an official GCC image
FROM gcc:11

# Set the working directory
WORKDIR /app

# Copy the current directory contents into the container
COPY . .

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libboost-all-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Build the application
RUN make

# Run the proxy server
CMD ["./proxy"]
