# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""Hand-written FR/ES/NL/PL UI translations for new strings."""

_UI_TRANSLATIONS: dict[str, tuple[str, str, str, str]] = {
    'GR': ('GR', 'GR', 'GR', 'GR'),
    'MAN': ('MAN', 'MAN', 'MAN', 'MAN'),
    'AUTO': ('AUTO', 'AUTO', 'AUTO', 'AUTO'),
    'Scan': ('Scanner', 'Escanear', 'Scannen', 'Skanuj'),
    'Idle': ('Inactif', 'Inactivo', 'Inactief', 'Bezczynny'),
    'Back': ('Retour', 'Atrás', 'Terug', 'Wstecz'),
    'Move': ('Déplacer', 'Mover', 'Verplaatsen', 'Przenieś'),
    'Level': ('Niveau', 'Nivel', 'Niveau', 'Poziom'),
    'Ready': ('Prêt', 'Listo', 'Gereed', 'Gotowe'),
    'Files': ('Fichiers', 'Archivos', 'Bestanden', 'Pliki'),
    'Never': ('Jamais', 'Nunca', 'Nooit', 'Nigdy'),
    'Forget': ('Oublier', 'Olvidar', 'Vergeten', 'Zapomnij'),
    '(none)': ('(aucun)', '(ninguno)', '(geen)', '(brak)'),
    'Export': ('Exporter', 'Exportar', 'Exporteren', 'Eksport'),
    'Series': ('Série', 'Serie', 'Serie', 'Seria'),
    'Alt IGC': ('Alt IGC', 'Alt IGC', 'Alt IGC', 'Alt IGC'),
    'Altitude IGC': ('Altitude IGC', 'Altitud IGC', 'Hoogte IGC',
                     'Wysokość IGC'),
    '%d secs': ('%d s', '%d s', '%d sec', '%d s'),
    'Connect': ('Connecter', 'Conectar', 'Verbinden', 'Połącz'),
    'Loading': ('Chargement', 'Cargando', 'Laden', 'Ładowanie'),
    'Partial': ('Partiel', 'Parcial', 'Gedeeltelijk', 'Częściowy'),
    'Flights': ('Vols', 'Vuelos', 'Vluchten', 'Loty'),
    'Restore': ('Restaurer', 'Restaurar', 'Herstellen', 'Przywróć'),
    'Folders': ('Dossiers', 'Carpetas', 'Mappen', 'Foldery'),
    'Expired': ('Expiré', 'Caducado', 'Verlopen', 'Wygasłe'),
    'API URL': ('URL API', 'URL de API', 'API-URL', 'URL API'),
    'Prev WP': ('WP préc.', 'WP ant.', 'Vorige WP', 'Poprz. PKT'),
    'VHF link': ('Lien VHF', 'Enlace VHF', 'VHF-link', 'Łącze VHF'),
    'Too late': ('Trop tard', 'Demasiado tarde', 'Te laat', 'Za późno'),
    'Forecast': ('Prévision', 'Pronóstico', 'Voorspelling', 'Prognoza'),
    'Complete': ('Terminé', 'Completado', 'Voltooid', 'Zakończono'),
    'Filtered': ('Filtré', 'Filtrado', 'Gefilterd', 'Przefiltrowane'),
    '%u total': ('%u total', '%u total', '%u totaal', '%u łącznie'),
    'WiFi List': ('Liste Wi-Fi', 'Lista Wi-Fi', 'WiFi-lijst',
                  'Lista Wi-Fi'),
    'Active WP': ('WP actif', 'WP activo', 'Actieve WP', 'Aktywny PKT'),
    'Importing': ('Importation', 'Importando', 'Importeren',
                  'Importowanie'),
    'Altn {} {}': ('Alt {} {}', 'Alt {} {}', 'Alt {} {}', 'Alt {} {}'),
    'Altn {} {} {}': ('Alt {} {} {}', 'Alt {} {} {}', 'Alt {} {} {}',
                      'Alt {} {} {}'),
    ' dist %d m': (' dist %d m', ' dist %d m', ' afstand %d m',
                   ' odległość %d m'),
    ' vertical ': (' vertical ', ' vertical ', ' verticaal ', ' pionowo '),
    'Disconnect': ('Déconnecter', 'Desconectar', 'Verbreken', 'Rozłącz'),
    'RASP layer': ('Couche RASP', 'Capa RASP', 'RASP-laag',
                   'Warstwa RASP'),
    'Cached day': ('Jour en cache', 'Día en caché', 'Dag in cache',
                   'Dzień w pamięci'),
    'Move files': ('Déplacer des fichiers', 'Mover archivos',
                   'Bestanden verplaatsen', 'Przenieś pliki'),
    'Copy files': ('Copier des fichiers', 'Copiar archivos',
                   'Bestanden kopiëren', 'Kopiuj pliki'),
    'Valid From': ('Valide à partir de', 'Válido desde', 'Geldig vanaf',
                   'Ważne od'),
    '%u visible': ('%u visibles', '%u visibles', '%u zichtbaar',
                   '%u widocznych'),
    'Loading...': ('Chargement...', 'Cargando...', 'Laden...',
                   'Ładowanie...'),
    'Information': ('Information', 'Información', 'Informatie',
                    'Informacja'),
    'Scanning...': ('Analyse...', 'Escaneando...', 'Scannen...',
                    'Skanowanie...'),
    'Map overlay': ('Superposition carte', 'Superposición de mapa',
                    'Kaartoverlay', 'Nakładka mapy'),
    'Import data': ('Importer des données', 'Importar datos',
                    'Gegevens importeren', 'Importuj dane'),
    'Destination': ('Destination', 'Destino', 'Bestemming', 'Miejsce docelowe'),
    'Rename file': ('Renommer le fichier', 'Renombrar archivo',
                    'Bestand hernoemen', 'Zmień nazwę pliku'),
    'Transfer to': ('Transférer vers', 'Transferir a', 'Overzetten naar',
                    'Przenieś do'),
    'Valid Until': ('Valide jusqu’au', 'Válido hasta', 'Geldig tot',
                    'Ważne do'),
    '%u filtered': ('%u filtrés', '%u filtrados', '%u gefilterd',
                    '%u odfiltrowanych'),
    'EDL weather': ('Météo EDL', 'Tiempo EDL', 'EDL-weer', 'Pogoda EDL'),
    'Total energy': ('Énergie totale', 'Energía total', 'Totale energie',
                     'Energia całkowita'),
    'Alternates 1': ('Dégagements 1', 'Alternativos 1', 'Alternatieven 1',
                     'Zapasowe 1'),
    'Alternates 2': ('Dégagements 2', 'Alternativos 2', 'Alternatieven 2',
                     'Zapasowe 2'),
    'WiFi Enabled': ('Wi-Fi activé', 'Wi-Fi activado', 'WiFi ingeschakeld',
                     'Wi-Fi włączone'),
    'Connectivity': ('Connectivité', 'Conectividad', 'Connectiviteit',
                     'Łączność'),
    'Auto advance': ('Avance auto', 'Avance automático', 'Automatisch verder',
                     'Automatyczne przejście'),
    'Precache day': ('Précharger un jour', 'Precargar día', 'Dag voorcachen',
                     'Wstępnie buforuj dzień'),
    "Delete '%s'?": ("Supprimer '%s' ?", "¿Eliminar '%s'?",
                     "'%s' verwijderen?", "Usunąć '%s'?"),
    'Grant access': ('Accorder l’accès', 'Conceder acceso',
                     'Toegang verlenen', 'Przyznaj dostęp'),
    'WiFi backend': ('Backend Wi-Fi', 'Backend Wi-Fi', 'WiFi-backend',
                     'Backend Wi-Fi'),
    'HTTP support': ('Support HTTP', 'Soporte HTTP', 'HTTP-ondersteuning',
                     'Obsługa HTTP'),
    'Uncompensated': ('Non compensé', 'No compensado', 'Ongecompenseerd',
                      'Nieskompensowany'),
    'Start unknown': ('Départ inconnu', 'Inicio desconocido',
                      'Start onbekend', 'Start nieznany'),
    'Exporting: %s': ('Exportation : %s', 'Exportando: %s',
                      'Exporteren: %s', 'Eksportowanie: %s'),
    'Create backup': ('Créer une sauvegarde', 'Crear copia de seguridad',
                      'Back-up maken', 'Utwórz kopię zapasową'),
    'Delete backup': ('Supprimer la sauvegarde',
                      'Eliminar copia de seguridad',
                      'Back-up verwijderen', 'Usuń kopię zapasową'),
    'Create folder': ('Créer un dossier', 'Crear carpeta',
                      'Map aanmaken', 'Utwórz folder'),
    'Invalid name.': ('Nom invalide.', 'Nombre no válido.',
                      'Ongeldige naam.', 'Nieprawidłowa nazwa.'),
    'Change folder': ('Changer de dossier', 'Cambiar carpeta',
                      'Map wijzigen', 'Zmień folder'),
    'NOTAM Support': ('Support NOTAM', 'Soporte NOTAM',
                      'NOTAM-ondersteuning', 'Obsługa NOTAM'),
    'Search Radius': ('Rayon de recherche', 'Radio de búsqueda',
                      'Zoekradius', 'Promień wyszukiwania'),
    'Hidden network': ('Réseau caché', 'Red oculta', 'Verborgen netwerk',
                       'Ukryta sieć'),
    'Backup manager': ('Gestionnaire de sauvegardes',
                       'Gestor de copias de seguridad',
                       'Back-upbeheer', 'Menedżer kopii zapasowych'),
    'Backup failed.': ('Échec de la sauvegarde.',
                       'Falló la copia de seguridad.',
                       'Back-up mislukt.',
                       'Tworzenie kopii zapasowej nie powiodło się.'),
    'Backup cancelled.': ('Sauvegarde annulée.',
                          'Copia de seguridad cancelada.',
                          'Back-up geannuleerd.',
                          'Kopia zapasowa anulowana.'),
    'Checking WiFi...': ('Vérification du Wi-Fi...',
                         'Comprobando Wi-Fi...',
                         'WiFi controleren...',
                         'Sprawdzanie Wi-Fi...'),
    'Checking network services...': ('Vérification des services réseau...',
                                     'Comprobando servicios de red...',
                                     'Netwerkdiensten controleren...',
                                     'Sprawdzanie usług sieciowych...'),
    'Choose location': ('Choisir l’emplacement', 'Elegir ubicación',
                        'Locatie kiezen', 'Wybierz lokalizację'),
    'Clean other days': ('Nettoyer les autres jours',
                         'Limpiar otros días',
                         'Andere dagen opschonen',
                         'Wyczyść pozostałe dni'),
    'ConnMan is not available': ('ConnMan n’est pas disponible',
                                 'ConnMan no está disponible',
                                 'ConnMan is niet beschikbaar',
                                 'ConnMan nie jest dostępny'),
    'NetworkManager is not available':
        ('NetworkManager n’est pas disponible',
         'NetworkManager no está disponible',
         'NetworkManager is niet beschikbaar',
         'NetworkManager nie jest dostępny'),
    'Cannot determine parent path.':
        ('Impossible de déterminer le chemin parent.',
         'No se puede determinar la ruta padre.',
         'Kan bovenliggend pad niet bepalen.',
         'Nie można określić ścieżki nadrzędnej.'),
    'Failed to create folder.': ('Échec de la création du dossier.',
                                 'Error al crear la carpeta.',
                                 'Map aanmaken mislukt.',
                                 'Nie udało się utworzyć folderu.'),
    'Failed to disable WiFi.': ('Échec de la désactivation du Wi-Fi.',
                                'Error al desactivar Wi‑Fi.',
                                'WiFi uitschakelen mislukt.',
                                'Nie udało się wyłączyć Wi‑Fi.'),
    'Failed to enable WiFi.': ('Échec de l’activation du Wi-Fi.',
                               'Error al activar Wi‑Fi.',
                               'WiFi inschakelen mislukt.',
                               'Nie udało się włączyć Wi‑Fi.'),
    'Failed to load NOTAMs': ('Échec du chargement des NOTAM',
                              'No se pudieron cargar los NOTAM',
                              'NOTAMs laden mislukt',
                              'Nie udało się wczytać NOTAM-ów'),
    'Failed to open airspace details':
        ('Échec de l’ouverture des détails de l’espace aérien',
         'No se pudieron abrir los detalles del espacio aéreo',
         'Luchtruimdetails openen mislukt',
         'Nie udało się otworzyć szczegółów przestrzeni powietrznej'),
    'Failed to acknowledge airspace warning':
        ('Échec de l’acquittement de l’alerte espace aérien',
         'No se pudo confirmar la alerta de espacio aéreo',
         'Luchtruimwaarschuwing bevestigen mislukt',
         'Nie udało się potwierdzić ostrzeżenia przestrzeni powietrznej'),
    'Failed to acknowledge airspace warning for day':
        ('Échec de l’acquittement de l’alerte espace aérien pour la journée',
         'No se pudo confirmar la alerta de espacio aéreo del día',
         'Luchtruimwaarschuwing voor de dag bevestigen mislukt',
         'Nie udało się potwierdzić dziennego ostrzeżenia przestrzeni '
         'powietrznej'),
    'Failed to re-enable airspace warning':
        ('Échec de la réactivation de l’alerte espace aérien',
         'No se pudo volver a activar la alerta de espacio aéreo',
         'Luchtruimwaarschuwing opnieuw inschakelen mislukt',
         'Nie udało się ponownie włączyć ostrzeżenia przestrzeni powietrznej'),
    'Failed to update airspace acknowledgement':
        ('Échec de la mise à jour de l’acquittement de l’espace aérien',
         'No se pudo actualizar la confirmación de espacio aéreo',
         'Luchtruimbevestiging bijwerken mislukt',
         'Nie udało się zaktualizować potwierdzenia przestrzeni powietrznej'),
    'Enable downloading and display of NOTAMs from aviation authorities.':
        ('Activer le téléchargement et l’affichage des NOTAM des autorités '
         'aéronautiques.',
         'Habilitar la descarga y visualización de NOTAM de autoridades '
         'aeronáuticas.',
         'Downloaden en tonen van NOTAMs van luchtvaartautoriteiten '
         'inschakelen.',
         'Włącz pobieranie i wyświetlanie NOTAM-ów od organów lotniczych.'),
    'Base URL of the NOTAM proxy API. Must be configured before NOTAMs can be fetched.':
        ('URL de base de l’API proxy NOTAM. Doit être configurée avant le '
         'récupération des NOTAM.',
         'URL base de la API proxy de NOTAM. Debe configurarse antes de '
         'obtener NOTAM.',
         'Basis-URL van de NOTAM-proxy-API. Moet worden ingesteld voordat '
         'NOTAMs kunnen worden opgehaald.',
         'Bazowy URL API proxy NOTAM. Musi być skonfigurowany przed '
         'pobraniem NOTAM-ów.'),
    'Automatically refresh NOTAMs every X minutes. Set to 0 to disable.':
        ('Actualiser automatiquement les NOTAM toutes les X minutes. '
         'Mettre 0 pour désactiver.',
         'Actualizar automáticamente los NOTAM cada X minutos. Ponga 0 para '
         'desactivar.',
         'NOTAMs elke X minuten automatisch verversen. Zet op 0 om uit te '
         'schakelen.',
         'Automatycznie odświeżaj NOTAM-y co X minut. Ustaw 0, aby wyłączyć.'),
    'Automatically follow the current local time in 15-minute steps.':
        ('Suivre automatiquement l’heure locale actuelle par pas de '
         '15 minutes.',
         'Seguir automáticamente la hora local actual en pasos de 15 minutos.',
         'Volg automatisch de huidige lokale tijd in stappen van 15 minuten.',
         'Automatycznie śledź bieżący czas lokalny co 15 minut.'),
    'After Filtering': ('Après filtrage', 'Tras filtrar', 'Na filteren',
                        'Po filtrowaniu'),
    'Auto-Refresh (minutes)': ('Auto-actualisation (minutes)',
                               'Autoactualización (minutos)',
                               'Auto-verversen (minuten)',
                               'Auto-odświeżanie (minuty)'),
    'Active Waypoint': ('Waypoint actif', 'Waypoint activo',
                        'Actief waypoint', 'Aktywny punkt trasy'),
    'Select as Alternate': ('Sélectionner comme dégagement',
                            'Seleccionar como alternativo',
                            'Selecteer als alternatief',
                            'Wybierz jako zapasowe'),
    'Altn mode: MANUAL': ('Mode alt : MANUEL', 'Modo alt: MANUAL',
                          'Alt-modus: HANDMATIG', 'Tryb alt: RĘCZNY'),
    'Altn mode: AUTO': ('Mode alt : AUTO', 'Modo alt: AUTO',
                        'Alt-modus: AUTO', 'Tryb alt: AUTO'),
    'User repositories': ('Dépôts utilisateur', 'Repositorios de usuario',
                          'Gebruikersrepositories', 'Repozytoria użytkownika'),
    "List of additional user repository URIs, separated by '|' character.":
        ("Liste des URI de dépôts utilisateur supplémentaires, séparées par "
         "le caractère '|'.",
         "Lista de URI de repositorios de usuario adicionales, separadas por "
         "el carácter '|'.",
         "Lijst met extra gebruikersrepository-URI's, gescheiden door het "
         "teken '|'.",
         "Lista dodatkowych URI repozytoriów użytkownika, rozdzielonych "
         "znakiem '|'."),
    'Fixed (North-West)': ('Fixe (Nord-Ouest)', 'Fijo (Noroeste)',
                           'Vast (Noordwest)', 'Stałe (północny zachód)'),
    'Fixed (Top Left)': ('Fixe (Haut gauche)', 'Fijo (Arriba izquierda)',
                         'Vast (Linksboven)', 'Stałe (lewy górny róg)'),
    'Waypoint icon size': ('Taille d’icône waypoint',
                           'Tamaño del icono de waypoint',
                           'Waypoint-pictogramgrootte',
                           'Rozmiar ikony punktu trasy'),
    'Please enter four letters or digits only (ICAO station code).':
        ('Veuillez saisir seulement quatre lettres ou chiffres (code station '
         'ICAO).',
         'Introduzca solo cuatro letras o dígitos (código de estación ICAO).',
         'Voer alleen vier letters of cijfers in (ICAO-stationcode).',
         'Wprowadź tylko cztery litery lub cyfry (kod stacji ICAO).'),
    'Updating repository': ('Mise à jour du dépôt',
                            'Actualizando repositorio',
                            'Repository bijwerken',
                            'Aktualizowanie repozytorium'),
    'Emit GPGGA/GPRMC': ('Émettre GPGGA/GPRMC', 'Emitir GPGGA/GPRMC',
                         'GPGGA/GPRMC uitzenden', 'Wysyłaj GPGGA/GPRMC'),
    'Set standby from a link': ('Définir standby depuis un lien',
                                'Establecer en espera desde un enlace',
                                'Standby via link instellen',
                                'Ustaw standby z linku'),
    'This frequency is not valid for VHF airband.':
        ('Cette fréquence n’est pas valide pour la bande VHF aviation.',
         'Esta frecuencia no es válida para la banda aérea VHF.',
         'Deze frequentie is niet geldig voor de VHF-luchtband.',
         'Ta częstotliwość jest nieprawidłowa dla pasma lotniczego VHF.'),
    'Add #standby or #active to the link, for example vhf:122.800#standby':
        ('Ajoutez #standby ou #active au lien, par exemple '
         'vhf:122.800#standby',
         'Añada #standby o #active al enlace, por ejemplo '
         'vhf:122.800#standby',
         'Voeg #standby of #active toe aan de link, bijvoorbeeld '
         'vhf:122.800#standby',
         'Dodaj #standby lub #active do linku, na przykład '
         'vhf:122.800#standby'),
    'PEV offset at start': ('Décalage PEV au départ',
                            'Desfase PEV al inicio',
                            'PEV-offset bij start',
                            'Przesunięcie PEV na starcie'),
    'Restoring will overwrite existing data. Continue?':
        ('La restauration écrasera les données existantes. Continuer ?',
         'La restauración sobrescribirá los datos existentes. ¿Continuar?',
         'Herstellen overschrijft bestaande gegevens. Doorgaan?',
         'Przywracanie nadpisze istniejące dane. Kontynuować?'),
    'Delete this backup?': ('Supprimer cette sauvegarde ?',
                            '¿Eliminar esta copia de seguridad?',
                            'Deze back-up verwijderen?',
                            'Usunąć tę kopię zapasową?'),
    'Creating backup': ('Création de la sauvegarde',
                        'Creando copia de seguridad',
                        'Back-up maken',
                        'Tworzenie kopii zapasowej'),
    'Restoring backup': ('Restauration de la sauvegarde',
                         'Restaurando copia de seguridad',
                         'Back-up herstellen',
                         'Przywracanie kopii zapasowej'),
    'No primary data path.': ('Aucun chemin de données principal.',
                              'No hay ruta de datos primaria.',
                              'Geen primair gegevenspad.',
                              'Brak głównej ścieżki danych.'),
    'Show .nmea files': ('Afficher les fichiers .nmea',
                         'Mostrar archivos .nmea',
                         '.nmea-bestanden tonen',
                         'Pokaż pliki .nmea'),
    'Only .igc files can be uploaded to WeGlide.':
        ('Seuls les fichiers .igc peuvent être envoyés vers WeGlide.',
         'Solo se pueden subir archivos .igc a WeGlide.',
         'Alleen .igc-bestanden kunnen naar WeGlide worden geüpload.',
         'Do WeGlide można przesyłać tylko pliki .igc.'),
    'WeGlide is not configured. Please set your pilot ID and birthdate in the settings.':
        ('WeGlide n’est pas configuré. Veuillez définir votre ID pilote et '
         'votre date de naissance dans les paramètres.',
         'WeGlide no está configurado. Configure su ID de piloto y fecha de '
         'nacimiento en los ajustes.',
         'WeGlide is niet geconfigureerd. Stel uw piloot-ID en geboortedatum '
         'in de instellingen in.',
         'WeGlide nie jest skonfigurowany. Ustaw identyfikator pilota i datę '
         'urodzenia w ustawieniach.'),
    'Uploaded %u file(s)': ('%u fichier(s) téléversé(s)',
                            '%u archivo(s) subido(s)',
                            '%u bestand(en) geüpload',
                            'Przesłano %u plik(ów)'),
    'Exported %u file(s)': ('%u fichier(s) exporté(s)',
                            '%u archivo(s) exportado(s)',
                            '%u bestand(en) geëxporteerd',
                            'Wyeksportowano %u plik(ów)'),
    'Organize files': ('Organiser les fichiers', 'Organizar archivos',
                       'Bestanden organiseren', 'Uporządkuj pliki'),
    'Advanced File Explorer': ('Explorateur de fichiers avancé',
                               'Explorador de archivos avanzado',
                               'Geavanceerde bestandsverkenner',
                               'Zaawansowany eksplorator plików'),
    'Download': ('Télécharger', 'Descargar', 'Downloaden', 'Pobierz'),
    'Import': ('Importer', 'Importar', 'Importeren', 'Import'),
    'Target': ('Cible', 'Destino', 'Doel', 'Cel'),
    'Source': ('Source', 'Origen', 'Bron', 'Źródło'),
    'D-Bus provider used for WiFi (see WiFi list to manage networks).':
        ('Fournisseur D-Bus utilisé pour le Wi-Fi (voir la liste Wi-Fi pour '
         'gérer les réseaux).',
         'Proveedor D-Bus usado para Wi‑Fi (consulte la lista Wi‑Fi para '
         'gestionar redes).',
         'D-Bus-provider gebruikt voor WiFi (zie WiFi-lijst om netwerken te '
         'beheren).',
         'Dostawca D-Bus używany dla Wi‑Fi (zobacz listę Wi‑Fi, aby zarządzać '
         'sieciami).'),
    'The selected network is not available. Try scanning again.':
        ('Le réseau sélectionné n’est pas disponible. Essayez de scanner '
         'à nouveau.',
         'La red seleccionada no está disponible. Intente escanear de nuevo.',
         'Het geselecteerde netwerk is niet beschikbaar. Probeer opnieuw te '
         'scannen.',
         'Wybrana sieć nie jest dostępna. Spróbuj zeskanować ponownie.'),
    'In-app network settings are not available in this build.':
        ('Les paramètres réseau intégrés ne sont pas disponibles dans cette '
         'version.',
         'Los ajustes de red en la aplicación no están disponibles en esta '
         'compilación.',
         'Netwerkinstellingen in de app zijn niet beschikbaar in deze build.',
         'Ustawienia sieci w aplikacji nie są dostępne w tej kompilacji.'),
    'Select WeGlide Type': ('Sélectionner le type WeGlide',
                            'Seleccionar tipo WeGlide',
                            'Selecteer WeGlide-type',
                            'Wybierz typ WeGlide'),
    'Thermal hotspot': ('Point chaud thermique', 'Punto caliente térmico',
                        'Thermische hotspot', 'Hotspot termiczny'),

    'Export flights': ('Exporter les vols', 'Exportar vuelos',
                       'Vluchten exporteren', 'Eksportuj loty'),
    'Restore backup': ('Restaurer la sauvegarde',
                       'Restaurar copia de seguridad',
                       'Back-up herstellen', 'Przywróć kopię zapasową'),
    'Transfer files': ('Transférer des fichiers', 'Transferir archivos',
                       'Bestanden overzetten', 'Przenieś pliki'),
    'Expired %s ago': ('Expiré il y a %s', 'Caducó hace %s',
                       '%s geleden verlopen', 'Wygasło %s temu'),
    '[Invalid text]': ('[Texte invalide]', '[Texto inválido]',
                       '[Ongeldige tekst]', '[Nieprawidłowy tekst]'),
    'Loaded 1 NOTAM': ('1 NOTAM chargé', '1 NOTAM cargado',
                       '1 NOTAM geladen', 'Wczytano 1 NOTAM'),
    'Rename failed.': ('Échec du renommage.', 'Error al renombrar.',
                       'Hernoemen mislukt.', 'Zmiana nazwy nie powiodła się.'),
    'Hidden Q-Codes': ('Q-codes masqués', 'Q-Codes ocultos',
                       'Verborgen Q-codes', 'Ukryte kody Q'),
    'Data Management': ('Gestion des données', 'Gestión de datos',
                        'Gegevensbeheer', 'Zarządzanie danymi'),
    'Restore failed.': ('Échec de la restauration.',
                        'La restauración falló.',
                        'Herstellen mislukt.',
                        'Przywracanie nie powiodło się.'),
    'Download manager': ('Gestionnaire de téléchargements',
                         'Gestor de descargas',
                         'Downloadbeheer',
                         'Menedżer pobierania'),
    'WeGlide Aircraft': ('Aéronef WeGlide', 'Aeronave WeGlide',
                         'WeGlide-luchtvaartuig', 'Statek powietrzny WeGlide'),
    'Delete %u items?': ('Supprimer %u éléments ?',
                         '¿Eliminar %u elementos?',
                         '%u items verwijderen?',
                         'Usunąć %u elementów?'),
    'Weather controls': ('Contrôles météo', 'Controles meteorológicos',
                         'Weerbediening', 'Sterowanie pogodą'),
    'Loaded %u NOTAMs': ('%u NOTAM chargés', '%u NOTAM cargados',
                         '%u NOTAMs geladen', 'Wczytano %u NOTAM-ów'),
    'Previous Waypoint': ('Waypoint précédent', 'Waypoint anterior',
                          'Vorig waypoint', 'Poprzedni punkt trasy'),
    'Finalizing backup': ('Finalisation de la sauvegarde',
                          'Finalizando copia de seguridad',
                          'Back-up afronden',
                          'Finalizowanie kopii zapasowej'),
    'Show NOTAM labels': ('Afficher les étiquettes NOTAM',
                          'Mostrar etiquetas NOTAM',
                          'NOTAM-labels tonen',
                          'Pokaż etykiety NOTAM'),
    'NOTAM render error': ('Erreur de rendu NOTAM', 'Error de renderizado NOTAM',
                           'NOTAM-weergavefout',
                           'Błąd renderowania NOTAM'),
    'WiFi update failed': ('Échec de mise à jour Wi-Fi',
                           'Falló la actualización de Wi‑Fi',
                           'WiFi-update mislukt',
                           'Aktualizacja Wi‑Fi nie powiodła się'),
    'Select backup file': ('Sélectionner le fichier de sauvegarde',
                           'Seleccionar archivo de copia de seguridad',
                           'Back-upbestand selecteren',
                           'Wybierz plik kopii zapasowej'),
    'Restore cancelled.': ('Restauration annulée.',
                           'Restauración cancelada.',
                           'Herstellen geannuleerd.',
                           'Przywracanie anulowane.'),
    'No valid location.': ('Aucun emplacement valide.',
                           'No hay ubicación válida.',
                           'Geen geldige locatie.',
                           'Brak prawidłowej lokalizacji.'),
    'Storage removed: {}': ('Stockage retiré : {}', 'Almacenamiento retirado: {}',
                            'Opslag verwijderd: {}',
                            'Nośnik usunięty: {}'),
    'No D-Bus connection': ('Aucune connexion D-Bus',
                            'Sin conexión D-Bus',
                            'Geen D-Bus-verbinding',
                            'Brak połączenia D-Bus'),
    'No NOTAMs available': ('Aucun NOTAM disponible',
                            'No hay NOTAM disponibles',
                            'Geen NOTAMs beschikbaar',
                            'Brak dostępnych NOTAM-ów'),
    'No RASP file loaded': ('Aucun fichier RASP chargé',
                            'No se cargó ningún archivo RASP',
                            'Geen RASP-bestand geladen',
                            'Nie wczytano pliku RASP'),
    'Last Update (local)': ('Dernière mise à jour (locale)',
                            'Última actualización (local)',
                            'Laatste update (lokaal)',
                            'Ostatnia aktualizacja (lokalnie)'),
    'NOTAM Affected Area': ('Zone affectée NOTAM',
                            'Área afectada por NOTAM',
                            'NOTAM-gebied',
                            'Obszar objęty NOTAM'),
    'Deleted %u item(s).': ('%u élément(s) supprimé(s).',
                            'Se eliminaron %u elemento(s).',
                            '%u item(s) verwijderd.',
                            'Usunięto %u element(ów).'),
    'Invalid folder name.': ('Nom de dossier invalide.',
                             'Nombre de carpeta no válido.',
                             'Ongeldige mapnaam.',
                             'Nieprawidłowa nazwa folderu.'),
    'Maximum NOTAM Radius': ('Rayon NOTAM maximum',
                             'Radio máximo de NOTAM',
                             'Maximale NOTAM-radius',
                             'Maksymalny promień NOTAM'),
    'Resume auto tracking': ('Reprendre le suivi auto',
                             'Reanudar seguimiento automático',
                             'Automatisch volgen hervatten',
                             'Wznów automatyczne śledzenie'),
    'Storage inserted: {}': ('Stockage inséré : {}', 'Almacenamiento insertado: {}',
                             'Opslag geplaatst: {}',
                             'Nośnik włożony: {}'),
    'Show IFR-Only NOTAMs': ('Afficher uniquement les NOTAM IFR',
                             'Mostrar solo NOTAM IFR',
                             'Toon alleen IFR-NOTAMs',
                             'Pokaż tylko NOTAM IFR'),
    '\nFailed %u.': ('\nÉchec %u.', '\nFallaron %u.',
                     '\nMislukt %u.', '\nNiepowodzenie %u.'),
    'Folder already exists.': ('Le dossier existe déjà.',
                               'La carpeta ya existe.',
                               'Map bestaat al.',
                               'Folder już istnieje.'),
    'Select a target first.': ('Sélectionnez d’abord une cible.',
                               'Seleccione primero un destino.',
                               'Selecteer eerst een doel.',
                               'Najpierw wybierz cel.'),
    'In-app network settings': ('Paramètres réseau intégrés',
                                'Ajustes de red en la aplicación',
                                'Netwerkinstellingen in app',
                                'Ustawienia sieci w aplikacji'),
    'Deleted %u cached files.': ('%u fichiers en cache supprimés.',
                                 'Se eliminaron %u archivos en caché.',
                                 '%u gecachte bestanden verwijderd.',
                                 'Usunięto %u plików z pamięci podręcznej.'),
    "Overwrite '%s' with '%s'?":
        ("Remplacer '%s' par '%s' ?",
         "¿Sobrescribir '%s' con '%s'?",
         "'%s' overschrijven met '%s'?",
         "Nadpisać '%s' plikiem '%s'?"),
    'Select at least one file.': ('Sélectionnez au moins un fichier.',
                                  'Seleccione al menos un archivo.',
                                  'Selecteer minstens één bestand.',
                                  'Wybierz co najmniej jeden plik.'),
    'No changes. %u device(s).': ('Aucun changement. %u appareil(s).',
                                  'Sin cambios. %u dispositivo(s).',
                                  'Geen wijzigingen. %u apparaat/apparaten.',
                                  'Brak zmian. %u urządzeń.'),
    "Passphrase of network '%s'":
        ("Phrase secrète du réseau '%s'",
         "Contraseña de la red '%s'",
         "Wachtwoordzin van netwerk '%s'",
         "Hasło sieci '%s'"),
    'Import data - choose files': ('Importer des données - choisir des fichiers',
                                   'Importar datos - elegir archivos',
                                   'Gegevens importeren - kies bestanden',
                                   'Import danych - wybierz pliki'),
    'Storage access granted: {}':
        ('Accès au stockage accordé : {}',
         'Acceso al almacenamiento concedido: {}',
         'Opslagtoegang verleend: {}',
         'Przyznano dostęp do pamięci: {}'),
    'No Wi-Fi backend available':
        ('Aucun backend Wi-Fi disponible',
         'No hay backend de Wi‑Fi disponible',
         'Geen WiFi-backend beschikbaar',
         'Brak dostępnego backendu Wi‑Fi'),
    'Invalid destination folder.': ('Dossier de destination invalide.',
                                    'Carpeta de destino no válida.',
                                    'Ongeldige doelmap.',
                                    'Nieprawidłowy folder docelowy.'),
    'Select at least one flight.': ('Sélectionnez au moins un vol.',
                                    'Seleccione al menos un vuelo.',
                                    'Selecteer minstens één vlucht.',
                                    'Wybierz co najmniej jeden lot.'),
    'Only files can be imported.': ('Seuls les fichiers peuvent être importés.',
                                    'Solo se pueden importar archivos.',
                                    'Alleen bestanden kunnen worden geïmporteerd.',
                                    'Importować można tylko pliki.'),
    'No Wi-Fi interface available': ('Aucune interface Wi-Fi disponible',
                                     'No hay interfaz Wi‑Fi disponible',
                                     'Geen WiFi-interface beschikbaar',
                                     'Brak dostępnego interfejsu Wi‑Fi'),
    'Show Only Currently Effective':
        ('Afficher seulement actuellement en vigueur',
         'Mostrar solo los actualmente vigentes',
         'Toon alleen momenteel geldige',
         'Pokaż tylko aktualnie obowiązujące'),
    'Import finished: %u imported.': ('Import terminé : %u importé(s).',
                                      'Importación terminada: %u importado(s).',
                                      'Import voltooid: %u geïmporteerd.',
                                      'Import zakończony: zaimportowano %u.'),
    'Navigation & Flight Resources':
        ('Navigation et ressources de vol',
         'Navegación y recursos de vuelo',
         'Navigatie en vluchtbronnen',
         'Nawigacja i zasoby lotu'),
    'Destination exists. Overwrite?': ('La destination existe. Écraser ?',
                                       'El destino existe. ¿Sobrescribir?',
                                       'Bestemming bestaat. Overschrijven?',
                                       'Miejsce docelowe istnieje. Nadpisać?'),
    'Deleted %u file(s). Failed %u.':
        ('%u fichier(s) supprimé(s). Échec %u.',
         'Se eliminaron %u archivo(s). Fallaron %u.',
         '%u bestand(en) verwijderd. Mislukt %u.',
         'Usunięto %u plik(ów). Niepowodzeń %u.'),
    'Select a single file to rename.':
        ('Sélectionnez un seul fichier à renommer.',
         'Seleccione un solo archivo para renombrar.',
         'Selecteer één bestand om te hernoemen.',
         'Wybierz pojedynczy plik do zmiany nazwy.'),
    "'%s' already exists. Overwrite?":
        ("'%s' existe déjà. Écraser ?",
         "'%s' ya existe. ¿Sobrescribir?",
         "'%s' bestaat al. Overschrijven?",
         "'%s' już istnieje. Nadpisać?"),
    'Found %u new device(s). %u total.':
        ('%u nouvel appareil trouvé(s). %u au total.',
         'Se encontraron %u dispositivo(s) nuevo(s). %u en total.',
         '%u nieuw(e) apparaat/apparaten gevonden. %u totaal.',
         'Znaleziono %u nowych urządzeń. Łącznie %u.'),
    'Removed %u device(s). %u remaining.':
        ('%u appareil(s) supprimé(s). %u restant(s).',
         'Se quitaron %u dispositivo(s). Quedan %u.',
         '%u apparaat/apparaten verwijderd. %u resterend.',
         'Usunięto %u urządzeń. Pozostało %u.'),
    'Current network connectivity state.':
        ('État actuel de connectivité réseau.',
         'Estado actual de conectividad de red.',
         'Huidige status van netwerkverbinding.',
         'Bieżący stan łączności sieciowej.'),
    'Include NOTAMs for IFR traffic only.':
        ('Inclure uniquement les NOTAM pour trafic IFR.',
         'Incluir NOTAM solo para tráfico IFR.',
         'Neem alleen NOTAMs voor IFR-verkeer op.',
         'Uwzględnij NOTAM-y tylko dla ruchu IFR.'),
    'Import canceled. Imported %u file(s).':
        ('Import annulé. %u fichier(s) importé(s).',
         'Importación cancelada. %u archivo(s) importado(s).',
         'Import geannuleerd. %u bestand(en) geïmporteerd.',
         'Import anulowany. Zaimportowano %u plik(ów).'),
    'Turns the Kobo WiFi interface on or off.':
        ('Active ou désactive l’interface Wi-Fi Kobo.',
         'Activa o desactiva la interfaz Wi‑Fi de Kobo.',
         'Schakelt de Kobo-WiFi-interface in of uit.',
         'Włącza lub wyłącza interfejs Wi‑Fi Kobo.'),
    'Moved %u file(s). Skipped %u. Failed %u.':
        ('%u fichier(s) déplacé(s). Ignoré(s) %u. Échec %u.',
         'Se movieron %u archivo(s). Omitidos %u. Fallaron %u.',
         '%u bestand(en) verplaatst. Overgeslagen %u. Mislukt %u.',
         'Przeniesiono %u plik(ów). Pominięto %u. Niepowodzeń %u.'),
    'Import finished: %u imported, %u failed.':
        ('Import terminé : %u importé(s), %u échec(s).',
         'Importación terminada: %u importado(s), %u fallaron.',
         'Import voltooid: %u geïmporteerd, %u mislukt.',
         'Import zakończony: zaimportowano %u, niepowodzeń %u.'),
    'Cached %u files for the selected UTC day.':
        ('%u fichiers mis en cache pour le jour UTC sélectionné.',
         'Se almacenaron en caché %u archivos para el día UTC seleccionado.',
         '%u bestanden gecachet voor de geselecteerde UTC-dag.',
         'Zbuforowano %u plików dla wybranego dnia UTC.'),
    'Copied %u file(s). Skipped %u. Failed %u.':
        ('%u fichier(s) copié(s). Ignoré(s) %u. Échec %u.',
         'Se copiaron %u archivo(s). Omitidos %u. Fallaron %u.',
         '%u bestand(en) gekopieerd. Overgeslagen %u. Mislukt %u.',
         'Skopiowano %u plik(ów). Pominięto %u. Niepowodzeń %u.'),
    'IPv4 address of the active WiFi interface.':
        ('Adresse IPv4 de l’interface Wi-Fi active.',
         'Dirección IPv4 de la interfaz Wi‑Fi activa.',
         'IPv4-adres van de actieve WiFi-interface.',
         'Adres IPv4 aktywnego interfejsu Wi‑Fi.'),
    'Filter out NOTAMs not currently in effect.':
        ('Filtrer les NOTAM non actuellement en vigueur.',
         'Filtrar NOTAM que no están actualmente vigentes.',
         'Filter NOTAMs die momenteel niet van kracht zijn.',
         'Odfiltruj NOTAM-y, które obecnie nie obowiązują.'),
    'WiFi on Kobo is managed by wpa_supplicant.':
        ('Le Wi-Fi sur Kobo est géré par wpa_supplicant.',
         'El Wi‑Fi en Kobo está gestionado por wpa_supplicant.',
         'WiFi op Kobo wordt beheerd door wpa_supplicant.',
         'Wi‑Fi na Kobo jest zarządzane przez wpa_supplicant.'),
    'No backup files found in selected location.':
        ('Aucun fichier de sauvegarde trouvé à l’emplacement sélectionné.',
         'No se encontraron archivos de copia en la ubicación seleccionada.',
         'Geen back-upbestanden gevonden op de geselecteerde locatie.',
         'Nie znaleziono plików kopii zapasowej w wybranej lokalizacji.'),
    'Optional weather overlay drawn on map pages.':
        ('Superposition météo optionnelle dessinée sur les pages carte.',
         'Capa meteorológica opcional dibujada en las páginas de mapa.',
         'Optionele weer-overlay getekend op kaartpagina’s.',
         'Opcjonalna nakładka pogodowa rysowana na stronach mapy.'),
    'RASP weather layer to display on this map page.':
        ('Couche météo RASP à afficher sur cette page carte.',
         'Capa meteorológica RASP a mostrar en esta página de mapa.',
         'RASP-weerlaag om op deze kaartpagina weer te geven.',
         'Warstwa pogodowa RASP do wyświetlenia na tej stronie mapy.'),
    'Radius around current location to fetch NOTAMs.':
        ('Rayon autour de la position actuelle pour récupérer les NOTAM.',
         'Radio alrededor de la ubicación actual para obtener NOTAM.',
         'Straal rond de huidige locatie om NOTAMs op te halen.',
         'Promień wokół bieżącej lokalizacji do pobierania NOTAM-ów.'),
    '\nRestart recommended for moved configured files.':
        ('\nRedémarrage recommandé pour les fichiers configurés déplacés.',
         '\nSe recomienda reiniciar para los archivos configurados movidos.',
         '\nHerstart aanbevolen voor verplaatste geconfigureerde bestanden.',
         '\nZalecany restart po przeniesieniu skonfigurowanych plików.'),
    'One or more selected files exceed 250 MB. Continue?':
        ('Un ou plusieurs fichiers sélectionnés dépassent 250 MB. Continuer ?',
         'Uno o más archivos seleccionados superan 250 MB. ¿Continuar?',
         'Een of meer geselecteerde bestanden zijn groter dan 250 MB. '
         'Doorgaan?',
         'Jeden lub więcej wybranych plików przekracza 250 MB. Kontynuować?'),
    'The selected import location is no longer available.':
        ('L’emplacement d’import sélectionné n’est plus disponible.',
         'La ubicación de importación seleccionada ya no está disponible.',
         'De geselecteerde importlocatie is niet meer beschikbaar.',
         'Wybrana lokalizacja importu nie jest już dostępna.'),
    'Subdirectory does not exist and could not be created.':
        ('Le sous-répertoire n’existe pas et n’a pas pu être créé.',
         'El subdirectorio no existe y no pudo crearse.',
         'Submap bestaat niet en kon niet worden aangemaakt.',
         'Podkatalog nie istnieje i nie udało się go utworzyć.'),
    'Could not connect. Check the passphrase and try again.':
        ('Connexion impossible. Vérifiez la phrase secrète et réessayez.',
         'No se pudo conectar. Compruebe la contraseña e inténtelo de nuevo.',
         'Kan niet verbinden. Controleer de wachtwoordzin en probeer opnieuw.',
         'Nie można połączyć. Sprawdź hasło i spróbuj ponownie.'),
    'Keep only %04u-%02u-%02u and delete the other cached days?':
        ('Conserver uniquement %04u-%02u-%02u et supprimer les autres jours '
         'en cache ?',
         '¿Conservar solo %04u-%02u-%02u y eliminar los otros días en caché?',
         'Alleen %04u-%02u-%02u behouden en de andere gecachte dagen '
         'verwijderen?',
         'Zachować tylko %04u-%02u-%02u i usunąć pozostałe zbuforowane dni?'),
    'Restore complete. Restart XCSoar to apply restored settings.':
        ('Restauration terminée. Redémarrez XCSoar pour appliquer les '
         'paramètres restaurés.',
         'Restauración completa. Reinicie XCSoar para aplicar la '
         'configuración restaurada.',
         'Herstellen voltooid. Herstart XCSoar om de herstelde instellingen '
         'toe te passen.',
         'Przywracanie zakończone. Uruchom ponownie XCSoar, aby zastosować '
         'przywrócone ustawienia.'),
    'No network service (NetworkManager or ConnMan) found on D-Bus.':
        ('Aucun service réseau (NetworkManager ou ConnMan) trouvé sur D-Bus.',
         'No se encontró servicio de red (NetworkManager o ConnMan) en D-Bus.',
         'Geen netwerkdienst (NetworkManager of ConnMan) gevonden op D-Bus.',
         'Nie znaleziono usługi sieciowej (NetworkManager lub ConnMan) w '
         'D-Bus.'),
    'Selected directories are skipped. Only files will be imported.':
        ('Les dossiers sélectionnés sont ignorés. Seuls les fichiers seront '
         'importés.',
         'Los directorios seleccionados se omiten. Solo se importarán '
         'archivos.',
         'Geselecteerde mappen worden overgeslagen. Alleen bestanden worden '
         'geïmporteerd.',
         'Wybrane katalogi są pomijane. Importowane będą tylko pliki.'),
    'Space-separated Q-code prefixes to hide (e.g., QA QK QN QOA QOL).':
        ('Préfixes Q-code séparés par des espaces à masquer (ex. QA QK QN QOA '
         'QOL).',
         'Prefijos de códigos Q separados por espacios para ocultar (p. ej., '
         'QA QK QN QOA QOL).',
         'Door spaties gescheiden Q-code-voorvoegsels om te verbergen '
         '(bijv. QA QK QN QOA QOL).',
         'Rozdzielone spacjami prefiksy kodów Q do ukrycia (np. QA QK QN QOA '
         'QOL).'),
    'Please set WeGlide Aircraft Type in Plane profile before uploading.':
        ('Veuillez définir le type d’aéronef WeGlide dans le profil Plane '
         'avant l’envoi.',
         'Configure el tipo de aeronave WeGlide en el perfil Plane antes de '
         'subir.',
         'Stel het WeGlide-vliegtuigtype in het vliegtuigprofiel in voor het '
         'uploaden.',
         'Przed przesłaniem ustaw typ statku powietrznego WeGlide w profilu '
         'samolotu.'),
    'Show brief NOTAM text labels on the map when zoomed in sufficiently.':
        ('Afficher de brèves étiquettes textuelles NOTAM sur la carte en zoom '
         'suffisant.',
         'Mostrar etiquetas breves de texto NOTAM en el mapa al acercar lo '
         'suficiente.',
         'Toon korte NOTAM-tekstlabels op de kaart wanneer voldoende '
         'ingezoomd.',
         'Pokaż krótkie etykiety tekstowe NOTAM na mapie przy '
         'wystarczającym powiększeniu.'),
    'Filter out NOTAMs with radius larger than this. Set to 0 to disable.':
        ('Filtrer les NOTAM dont le rayon est supérieur à cette valeur. '
         'Mettre 0 pour désactiver.',
         'Filtrar NOTAM con radio mayor que este valor. Ponga 0 para '
         'desactivar.',
         'Filter NOTAMs met een grotere straal dan deze waarde. Zet op 0 om '
         'uit te schakelen.',
         'Odfiltruj NOTAM-y o promieniu większym niż ta wartość. Ustaw 0, '
         'aby wyłączyć.'),
    'This page shows Kobo WiFi status. Open WiFi list to scan and connect.':
        ('Cette page affiche l’état Wi-Fi Kobo. Ouvrez la liste Wi-Fi pour '
         'rechercher et vous connecter.',
         'Esta página muestra el estado de Wi‑Fi de Kobo. Abra la lista '
         'Wi‑Fi para escanear y conectar.',
         'Deze pagina toont de Kobo-WiFi-status. Open de WiFi-lijst om te '
         'scannen en verbinden.',
         'Ta strona pokazuje stan Wi‑Fi Kobo. Otwórz listę Wi‑Fi, aby '
         'wyszukać i połączyć.'),
    'Permission denied. You may not be allowed to connect to this network.':
        ('Permission refusée. Vous n’êtes peut-être pas autorisé à vous '
         'connecter à ce réseau.',
         'Permiso denegado. Puede que no se le permita conectarse a esta red.',
         'Toestemming geweigerd. Mogelijk mag u geen verbinding maken met dit '
         'netwerk.',
         'Odmowa uprawnień. Możesz nie mieć prawa do połączenia z tą siecią.'),
    'A D-Bus error occurred. Is NetworkManager or ConnMan running on the bus?':
        ('Une erreur D-Bus est survenue. NetworkManager ou ConnMan fonctionne '
         't-il sur le bus ?',
         'Ocurrió un error de D-Bus. ¿Está NetworkManager o ConnMan '
         'ejecutándose en el bus?',
         'Er is een D-Bus-fout opgetreden. Draait NetworkManager of ConnMan '
         'op de bus?',
         'Wystąpił błąd D-Bus. Czy NetworkManager lub ConnMan działa na '
         'magistrali?'),
    'Use #standby or #active after the frequency (example: vhf:122.800#standby).':
        ('Utilisez #standby ou #active après la fréquence (exemple : '
         'vhf:122.800#standby).',
         'Use #standby o #active después de la frecuencia (ejemplo: '
         'vhf:122.800#standby).',
         'Gebruik #standby of #active na de frequentie (voorbeeld: '
         'vhf:122.800#standby).',
         'Użyj #standby lub #active po częstotliwości (przykład: '
         'vhf:122.800#standby).'),
    'Restore finished with %u failed files. Restart XCSoar to apply restored settings.':
        ('Restauration terminée avec %u fichiers en échec. Redémarrez XCSoar '
         'pour appliquer les paramètres restaurés.',
         'La restauración terminó con %u archivos fallidos. Reinicie XCSoar '
         'para aplicar la configuración restaurada.',
         'Herstellen voltooid met %u mislukte bestanden. Herstart XCSoar om '
         'de herstelde instellingen toe te passen.',
         'Przywracanie zakończono z %u nieudanymi plikami. Uruchom ponownie '
         'XCSoar, aby zastosować przywrócone ustawienia.'),
    'A passphrase is required, or a saved network for this name must exist in the system settings.':
        ('Une phrase secrète est requise, ou un réseau enregistré pour ce nom '
         'doit exister dans les paramètres système.',
         'Se requiere una contraseña, o debe existir una red guardada con ese '
         'nombre en la configuración del sistema.',
         'Een wachtwoordzin is vereist, of er moet een opgeslagen netwerk '
         'met deze naam bestaan in de systeeminstellingen.',
         'Wymagane jest hasło albo w ustawieniach systemowych musi istnieć '
         'zapisana sieć o tej nazwie.'),
    'Automatically follow the next UTC forecast hour and the next lower pressure level below the aircraft.':
        ('Suivre automatiquement la prochaine heure de prévision UTC et le '
         'niveau de pression inférieur suivant sous l’aéronef.',
         'Seguir automáticamente la siguiente hora de pronóstico UTC y el '
         'siguiente nivel de presión inferior por debajo de la aeronave.',
         'Volg automatisch het volgende UTC-voorspellingsuur en het volgende '
         'lagere drukniveau onder het vliegtuig.',
         'Automatycznie śledź następną godzinę prognozy UTC i kolejny niższy '
         'poziom ciśnienia poniżej statku powietrznego.'),
    'Notice: NOTAM display is for situational awareness only\nand does not replace proper pre-flight NOTAM briefing.':
        ('Remarque : l’affichage des NOTAM est uniquement destiné à la '
         'conscience situationnelle\net ne remplace pas un briefing NOTAM '
         'pré-vol approprié.',
         'Aviso: la visualización de NOTAM es solo para conciencia situacional\n'
         'y no sustituye un briefing NOTAM previo al vuelo adecuado.',
         'Let op: de NOTAM-weergave is alleen voor situationeel bewustzijn\n'
         'en vervangt geen correcte NOTAM-briefing vóór de vlucht.',
         'Uwaga: wyświetlanie NOTAM służy wyłącznie orientacji sytuacyjnej\n'
         'i nie zastępuje właściwego przedlotowego briefingu NOTAM.'),
    '\nSkipped %u existing.': ('\nIgnoré %u existant(s).',
                               '\nSe omitieron %u existentes.',
                               '\n%u bestaande overgeslagen.',
                               '\nPominięto %u istniejących.'),
    'Organize %u file(s)?\nSkipped unknown: %u\nSkipped conflicts: %u':
        ('Organiser %u fichier(s) ?\nIgnorés inconnus : %u\n'
         'Ignorés conflits : %u',
         '¿Organizar %u archivo(s)?\nOmitidos desconocidos: %u\n'
         'Omitidos por conflictos: %u',
         '%u bestand(en) organiseren?\nOvergeslagen onbekend: %u\n'
         'Overgeslagen conflicten: %u',
         'Uporządkować %u plik(ów)?\nPominięto nieznane: %u\n'
         'Pominięto konflikty: %u'),
    'No files to organize.\nSkipped unknown: %u\nSkipped conflicts: %u':
        ('Aucun fichier à organiser.\nIgnorés inconnus : %u\n'
         'Ignorés conflits : %u',
         'No hay archivos para organizar.\nOmitidos desconocidos: %u\n'
         'Omitidos por conflictos: %u',
         'Geen bestanden om te organiseren.\nOvergeslagen onbekend: %u\n'
         'Overgeslagen conflicten: %u',
         'Brak plików do uporządkowania.\nPominięto nieznane: %u\n'
         'Pominięto konflikty: %u'),
    'Organized %u file(s). Failed %u.\nSkipped unknown: %u\nSkipped conflicts: %u':
        ('%u fichier(s) organisé(s). Échec %u.\nIgnorés inconnus : %u\n'
         'Ignorés conflits : %u',
         'Se organizaron %u archivo(s). Fallaron %u.\n'
         'Omitidos desconocidos: %u\nOmitidos por conflictos: %u',
         '%u bestand(en) georganiseerd. Mislukt %u.\n'
         'Overgeslagen onbekend: %u\nOvergeslagen conflicten: %u',
         'Uporządkowano %u plik(ów). Niepowodzeń %u.\n'
         'Pominięto nieznane: %u\nPominięto konflikty: %u'),
}

FR_UI: dict[str, str] = {k: v[0] for k, v in _UI_TRANSLATIONS.items()}
ES_UI: dict[str, str] = {k: v[1] for k, v in _UI_TRANSLATIONS.items()}
NL_UI: dict[str, str] = {k: v[2] for k, v in _UI_TRANSLATIONS.items()}
PL_UI: dict[str, str] = {k: v[3] for k, v in _UI_TRANSLATIONS.items()}
