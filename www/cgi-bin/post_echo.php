<?php
$body = file_get_contents("php://stdin");
echo "Content-Type: text/plain\r\n\r\n";
echo "=== ENV (php) ===\n";
echo "REQUEST_METHOD=" . getenv("REQUEST_METHOD") . "\n";
echo "CONTENT_LENGTH=" . getenv("CONTENT_LENGTH") . "\n";
echo "QUERY_STRING=" . getenv("QUERY_STRING") . "\n";
echo "\n=== BODY ===\n";
echo $body . "\n";
