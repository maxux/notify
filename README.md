# notify
send a notification to `notified` server

# options
        --host     notified server host
        --port     notified server port (default 5050)
        --source   notification source name
        --level    importance (low, normal, critical)
        --title    notification title (optional)
        --message  notification message
        --tag      internal tag of the message
        --fork     send message in background (non blocking)
        --backup   send to this host if server is not responding
        --help     print this message

# dependancies
        jansson
