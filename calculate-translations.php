#!/bin/php
<?php

// Variables
$translationTemplateFile       = __DIR__ . '/i18n/translations.pot';
$translationPoFileMask         = __DIR__ . '/i18n/translations_*.po';
$translationDocumentOutputFile = __DIR__ . '/docs/translations.md';

// Get total translatable strings from POT
$totalStrings = ((int) shell_exec("msgattrib --no-obsolete " . \escapeshellarg($translationTemplateFile) . " | grep -c '^msgid '")) - 1;
if ($totalStrings <= 0) {
    echo ("[-] No strings found in POT file.\n");
    exit(1);
} else {
    echo ("[+] Total translatable strings in POT file: $totalStrings\n");
}

// Map language codes to stats
$poStats = [];

// Loop trough all PO files
foreach (glob($translationPoFileMask) as $poFile) {

    // Try and get the language code
    if (preg_match('/translations_([a-zA-Z0-9\-]+)\.po$/', basename($poFile), $matches)) {

        // Get code
        $code = strtoupper($matches[1]);

        // Get amount of translated strings
        $translated = (int) shell_exec("msgattrib --no-fuzzy --translated --no-obsolete " . escapeshellarg($poFile) . " | grep -c '^msgid '");

        // Remove metadata from translated count
        if ($translated > 0) {
            $translated--;
        }

        // Get percentage
        $percent = $totalStrings > 0 ? \number_format(($translated / $totalStrings) * 100, 1) : 0;

        // Remove decimals from round numbers
        if ($percent == (int) $percent) {
            $percent = (int) $percent;
        }

        // Store into array
        $poStats[$code] = [
            'done'    => $translated,
            'total'   => $totalStrings,
            'percent' => $percent,
        ];

        echo "[+] " . str_pad("$code:", 7, " ", STR_PAD_RIGHT) . "\t$percent%\t$translated/$totalStrings\n";
    }
}

// Make sure we have some calculations
if (empty($poStats)) {
    echo "[-] No calculations done\n";
    exit(1);
}

// Tell user we are going to update translation document now
echo "[>] Updating translation document...\n";

// Get document contents
$document = \file_get_contents($translationDocumentOutputFile);
if (empty($document)) {
    echo "[-] Failed to get contents of translation document\n";
    exit(1);
}

// Loop trough all languages
foreach ($poStats as $code => $stats) {

    // Replace percentage and totals for the language in the document
    $percentString = \str_pad($stats['percent'] . "%", 10, " ", STR_PAD_RIGHT);
    $countString   = \str_pad($stats['done'] . "/" . $stats['total'], 15, " ", STR_PAD_RIGHT);
    $pattern       = '/^(\|.+?\|\s' . preg_quote($code, '/') . '\s+?\|\s)(.+?)(\|\s)(.+?)(\|)/m';
    $replacement   = '${1}' . $percentString . '${3}' . $countString . '${5}';
    $document      = preg_replace($pattern, $replacement, $document);

    // Update file
    if (\file_put_contents($translationDocumentOutputFile, $document) == false) {
        echo "[-] Failed to update translation document\n";
        exit(1);
    }
}

echo "[+] Done!\n";
