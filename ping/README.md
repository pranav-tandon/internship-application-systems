# Cloudflare Internship Application: Systems

## How to Build
See `Makefile` for the build scripts within `ping`

To build the command line example, simply run `make` or `make compile`
at the command line in the current directory.

```
make
sudo ./ping www.cloudflare.com
```

## Examples
# ping HTTPS
```
sudo ./ping www.cloudflare.com
```

# ping HTTPS with TTL set to given size
```
sudo ./ping -t 100 www.cloudflare.com
```

## What is it?

Please write a small Ping CLI application for MacOS or Linux.
The CLI app should accept a hostname or an IP address as its argument, then send ICMP "echo requests" in a loop to the target while receiving "echo reply" messages.
It should report loss and RTT times for each sent message.
