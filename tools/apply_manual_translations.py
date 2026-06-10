#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""Apply hand-written translations (no LLM) to all po files."""

from __future__ import annotations

import glob
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import polib

SCRIPT_DIR = Path(__file__).resolve().parent

from manual_de_ui import DE_UI
from fr_es_nl_pl import FR_UI, ES_UI, NL_UI, PL_UI
from it_pt_ru_uk import IT_UI, PT_UI, PT_BR_UI, RU_UI, UK_UI
from cjk import JA_UI, KO_UI, ZH_CN_UI, ZH_HANT_UI
from nordic_slavic import (
    CS_UI, DA_UI, NB_UI, SV_UI, FI_UI, SK_UI, SL_UI, HR_UI, SR_UI,
    BG_UI, RO_UI, HU_UI, LT_UI, CA_UI, EL_UI, TR_UI, VI_UI, HE_UI, TE_UI,
)
from nordic_extra import (
    CS_UI as CS_X, DA_UI as DA_X, NB_UI as NB_X, SV_UI as SV_X, FI_UI as FI_X,
    SK_UI as SK_X, SL_UI as SL_X, HR_UI as HR_X, SR_UI as SR_X, BG_UI as BG_X,
    RO_UI as RO_X, HU_UI as HU_X, LT_UI as LT_X, CA_UI as CA_X, EL_UI as EL_X,
    TR_UI as TR_X, VI_UI as VI_X, HE_UI as HE_X, TE_UI as TE_X,
    IT_UI as IT_X, PT_UI as PT_X, PT_BR_UI as PT_BR_X, RU_UI as RU_X,
    UK_UI as UK_X,
)
from common_simple import COMMON_SIMPLE
from long_help import LONG_HELP

# Shared FormatText and dedup strings per language code.
SHARED: dict[str, dict[str, str]] = {
    'bg': {
        '%s is not available in this build.':
            '%s не е наличен в тази версия.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s не е наличен в тази версия, защото OpenGL рендерерът не е наличен.',
        '%s device not found.': '%s устройството не е намерено.',
        'The operation failed.': 'Операцията не бе успешна.',
        'Starts in %s': 'Започва след %s',
        ' above ground': ' над земята',
    },
    'ca': {
        '%s is not available in this build.':
            '%s no està disponible en aquesta compilació.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s no està disponible en aquesta compilació perquè el renderitzador OpenGL no està disponible.',
        '%s device not found.': "No s'ha trobat el dispositiu %s.",
        'The operation failed.': "L'operació ha fallat.",
        'Starts in %s': 'Comença d\'aquí a %s',
        ' above ground': ' sobre el terra',
    },
    'cs': {
        '%s is not available in this build.':
            '%s není v této sestavě k dispozici.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s není v této sestavě k dispozici, protože renderer OpenGL není k dispozici.',
        '%s device not found.': 'Zařízení %s nebylo nalezeno.',
        'The operation failed.': 'Operace selhala.',
        'Starts in %s': 'Začíná za %s',
        ' above ground': ' nad zemí',
    },
    'da': {
        '%s is not available in this build.':
            '%s er ikke tilgængelig i denne build.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s er ikke tilgængelig i denne build, fordi OpenGL-rendereren ikke er tilgængelig.',
        '%s device not found.': '%s-enhed ikke fundet.',
        'The operation failed.': 'Handlingen mislykkedes.',
        'Starts in %s': 'Starter om %s',
        ' above ground': ' over jorden',
    },
    'de': {
        '%s is not available in this build.':
            '%s ist in diesem Build nicht verfügbar.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s ist in diesem Build nicht verfügbar, weil der OpenGL-Renderer nicht verfügbar ist.',
        '%s device not found.': '%s-Gerät nicht gefunden.',
        'The operation failed.': 'Der Vorgang ist fehlgeschlagen.',
        'Starts in %s': 'Beginnt in %s',
        ' above ground': ' über Grund',
    },
    'el': {
        '%s is not available in this build.':
            'Το %s δεν είναι διαθέσιμο σε αυτήν την έκδοση.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'Το %s δεν είναι διαθέσιμο σε αυτήν την έκδοση επειδή ο αποδίδων OpenGL δεν είναι διαθέσιμος.',
        '%s device not found.': 'Η συσκευή %s δεν βρέθηκε.',
        'The operation failed.': 'Η λειτουργία απέτυχε.',
        'Starts in %s': 'Ξεκινά σε %s',
        ' above ground': ' πάνω από το έδαφος',
    },
    'es': {
        '%s is not available in this build.':
            '%s no está disponible en esta compilación.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s no está disponible en esta compilación porque el renderizador OpenGL no está disponible.',
        '%s device not found.': 'Dispositivo %s no encontrado.',
        'The operation failed.': 'La operación ha fallado.',
        'Starts in %s': 'Empieza en %s',
        ' above ground': ' sobre el suelo',
    },
    'fi': {
        '%s is not available in this build.':
            '%s ei ole saatavilla tässä kokoelmassa.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s ei ole saatavilla tässä kokoelmassa, koska OpenGL-renderöijä ei ole saatavilla.',
        '%s device not found.': 'Laitetta %s ei löytynyt.',
        'The operation failed.': 'Toiminto epäonnistui.',
        'Starts in %s': 'Alkaa %s kuluttua',
        ' above ground': ' maanpinnan yläpuolella',
    },
    'fr': {
        '%s is not available in this build.':
            "%s n'est pas disponible dans ce build.",
        '%s is not available in this build because the OpenGL renderer is not available.':
            "%s n'est pas disponible dans ce build, car le moteur de rendu OpenGL n'est pas disponible.",
        '%s device not found.': 'Périphérique %s introuvable.',
        'The operation failed.': "L'opération a échoué.",
        'Starts in %s': 'Commence dans %s',
        ' above ground': ' au-dessus du sol',
    },
    'he': {
        '%s is not available in this build.':
            '%s אינו זמין בגרסה זו.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s אינו זמין בגרסה זו כי מנוע העיבוד OpenGL אינו זמין.',
        '%s device not found.': 'התקן %s לא נמצא.',
        'The operation failed.': 'הפעולה נכשלה.',
        'Starts in %s': 'מתחיל בעוד %s',
        ' above ground': ' מעל הקרקע',
    },
    'hr': {
        '%s is not available in this build.':
            '%s nije dostupan u ovoj verziji.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s nije dostupan u ovoj verziji jer OpenGL renderer nije dostupan.',
        '%s device not found.': 'Uređaj %s nije pronađen.',
        'The operation failed.': 'Operacija nije uspjela.',
        'Starts in %s': 'Počinje za %s',
        ' above ground': ' iznad tla',
    },
    'hu': {
        '%s is not available in this build.':
            'A(z) %s nem érhető el ebben a buildben.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'A(z) %s nem érhető el ebben a buildben, mert az OpenGL megjelenítő nem érhető el.',
        '%s device not found.': 'A(z) %s eszköz nem található.',
        'The operation failed.': 'A művelet sikertelen.',
        'Starts in %s': 'Indulás %s múlva',
        ' above ground': ' a talaj felett',
    },
    'it': {
        '%s is not available in this build.':
            '%s non è disponibile in questa build.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s non è disponibile in questa build perché il renderer OpenGL non è disponibile.',
        '%s device not found.': 'Dispositivo %s non trovato.',
        'The operation failed.': "L'operazione non è riuscita.",
        'Starts in %s': 'Inizia tra %s',
        ' above ground': ' sopra il suolo',
    },
    'ja': {
        '%s is not available in this build.':
            '%s はこのビルドでは利用できません。',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'OpenGL レンダラーが利用できないため、%s はこのビルドでは利用できません。',
        '%s device not found.': '%s デバイスが見つかりません。',
        'The operation failed.': '操作に失敗しました。',
        'Starts in %s': '%s 後に開始',
        ' above ground': ' 地上',
    },
    'ko': {
        '%s is not available in this build.':
            '%s은(는) 이 빌드에서 사용할 수 없습니다.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'OpenGL 렌더러를 사용할 수 없어 %s은(는) 이 빌드에서 사용할 수 없습니다.',
        '%s device not found.': '%s 장치를 찾을 수 없습니다.',
        'The operation failed.': '작업에 실패했습니다.',
        'Starts in %s': '%s 후 시작',
        ' above ground': ' 지상',
    },
    'lt': {
        '%s is not available in this build.':
            '%s šiame build\'e nepasiekiamas.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s šiame build\'e nepasiekiamas, nes OpenGL atvaizdavimo variklis nepasiekiamas.',
        '%s device not found.': '%s įrenginys nerastas.',
        'The operation failed.': 'Operacija nepavyko.',
        'Starts in %s': 'Prasideda po %s',
        ' above ground': ' virš žemės',
    },
    'nb': {
        '%s is not available in this build.':
            '%s er ikke tilgjengelig i denne builden.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s er ikke tilgjengelig i denne builden fordi OpenGL-rendereren ikke er tilgjengelig.',
        '%s device not found.': '%s-enhet ikke funnet.',
        'The operation failed.': 'Operasjonen mislyktes.',
        'Starts in %s': 'Starter om %s',
        ' above ground': ' over bakken',
    },
    'nl': {
        '%s is not available in this build.':
            '%s is niet beschikbaar in deze build.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s is niet beschikbaar in deze build omdat de OpenGL-renderer niet beschikbaar is.',
        '%s device not found.': '%s-apparaat niet gevonden.',
        'The operation failed.': 'De bewerking is mislukt.',
        'Starts in %s': 'Begint over %s',
        ' above ground': ' boven de grond',
    },
    'pl': {
        '%s is not available in this build.':
            '%s nie jest dostępny w tej kompilacji.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s nie jest dostępny w tej kompilacji, ponieważ renderer OpenGL nie jest dostępny.',
        '%s device not found.': 'Nie znaleziono urządzenia %s.',
        'The operation failed.': 'Operacja nie powiodła się.',
        'Starts in %s': 'Rozpoczyna się za %s',
        ' above ground': ' nad ziemią',
    },
    'pt': {
        '%s is not available in this build.':
            '%s não está disponível nesta compilação.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s não está disponível nesta compilação porque o renderizador OpenGL não está disponível.',
        '%s device not found.': 'Dispositivo %s não encontrado.',
        'The operation failed.': 'A operação falhou.',
        'Starts in %s': 'Começa em %s',
        ' above ground': ' acima do solo',
    },
    'pt_BR': {
        '%s is not available in this build.':
            '%s não está disponível nesta compilação.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s não está disponível nesta compilação porque o renderizador OpenGL não está disponível.',
        '%s device not found.': 'Dispositivo %s não encontrado.',
        'The operation failed.': 'A operação falhou.',
        'Starts in %s': 'Começa em %s',
        ' above ground': ' acima do solo',
    },
    'ro': {
        '%s is not available in this build.':
            '%s nu este disponibil în această compilare.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s nu este disponibil în această compilare deoarece rendererul OpenGL nu este disponibil.',
        '%s device not found.': 'Dispozitivul %s nu a fost găsit.',
        'The operation failed.': 'Operațiunea a eșuat.',
        'Starts in %s': 'Începe în %s',
        ' above ground': ' deasupra solului',
    },
    'ru': {
        '%s is not available in this build.':
            '%s недоступен в этой сборке.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s недоступен в этой сборке, так как OpenGL-рендерер недоступен.',
        '%s device not found.': 'Устройство %s не найдено.',
        'The operation failed.': 'Операция не удалась.',
        'Starts in %s': 'Начнётся через %s',
        ' above ground': ' над землёй',
    },
    'sk': {
        '%s is not available in this build.':
            '%s nie je v tejto zostave k dispozícii.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s nie je v tejto zostave k dispozícii, pretože renderer OpenGL nie je k dispozícii.',
        '%s device not found.': 'Zariadenie %s sa nenašlo.',
        'The operation failed.': 'Operácia zlyhala.',
        'Starts in %s': 'Začína o %s',
        ' above ground': ' nad zemou',
    },
    'sl': {
        '%s is not available in this build.':
            '%s ni na voljo v tej različici.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s ni na voljo v tej različici, ker OpenGL upodabljalnik ni na voljo.',
        '%s device not found.': 'Naprava %s ni bila najdena.',
        'The operation failed.': 'Operacija ni uspela.',
        'Starts in %s': 'Začne se čez %s',
        ' above ground': ' nad tlemi',
    },
    'sr': {
        '%s is not available in this build.':
            '%s није доступан у овој верзији.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s није доступан у овој верзији јер OpenGL рендерер није доступан.',
        '%s device not found.': 'Уређај %s није пронађен.',
        'The operation failed.': 'Операција није успела.',
        'Starts in %s': 'Почиње за %s',
        ' above ground': ' iznad tla',
    },
    'sv': {
        '%s is not available in this build.':
            '%s är inte tillgänglig i denna build.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s är inte tillgänglig i denna build eftersom OpenGL-renderaren inte är tillgänglig.',
        '%s device not found.': '%s-enhet hittades inte.',
        'The operation failed.': 'Åtgärden misslyckades.',
        'Starts in %s': 'Startar om %s',
        ' above ground': ' ovan mark',
    },
    'te': {
        '%s is not available in this build.':
            '%s ఈ బిల్డ్‌లో అందుబాటులో లేదు.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'OpenGL రెండరర్ అందుబాటులో లేనందున %s ఈ బిల్డ్‌లో అందుబాటులో లేదు.',
        '%s device not found.': '%s పరికరం కనుగొనబడలేదు.',
        'The operation failed.': 'ఆపరేషన్ విఫలమైంది.',
        'Starts in %s': '%s లో ప్రారంభమవుతుంది',
        ' above ground': ' నేలపై',
    },
    'tr': {
        '%s is not available in this build.':
            '%s bu derlemede kullanılamaz.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            'OpenGL oluşturucu kullanılamadığı için %s bu derlemede kullanılamaz.',
        '%s device not found.': '%s aygıtı bulunamadı.',
        'The operation failed.': 'İşlem başarısız oldu.',
        'Starts in %s': '%s içinde başlar',
        ' above ground': ' yer üstünde',
    },
    'uk': {
        '%s is not available in this build.':
            '%s недоступний у цій збірці.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s недоступний у цій збірці, оскільки OpenGL-рендерер недоступний.',
        '%s device not found.': 'Пристрій %s не знайдено.',
        'The operation failed.': 'Операція не вдалася.',
        'Starts in %s': 'Починається через %s',
        ' above ground': ' над землею',
    },
    'vi': {
        '%s is not available in this build.':
            '%s không có sẵn trong bản dựng này.',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '%s không có sẵn trong bản dựng này vì trình kết xuất OpenGL không có sẵn.',
        '%s device not found.': 'Không tìm thấy thiết bị %s.',
        'The operation failed.': 'Thao tác thất bại.',
        'Starts in %s': 'Bắt đầu sau %s',
        ' above ground': ' trên mặt đất',
    },
    'zh_CN': {
        '%s is not available in this build.':
            '%s 在此版本中不可用。',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '由于 OpenGL 渲染器不可用，%s 在此版本中不可用。',
        '%s device not found.': '未找到 %s 设备。',
        'The operation failed.': '操作失败。',
        'Starts in %s': '%s 后开始',
        ' above ground': ' 地面以上',
    },
    'zh_Hant': {
        '%s is not available in this build.':
            '%s 在此版本中不可用。',
        '%s is not available in this build because the OpenGL renderer is not available.':
            '由於 OpenGL 渲染器不可用，%s 在此版本中不可用。',
        '%s device not found.': '找不到 %s 裝置。',
        'The operation failed.': '操作失敗。',
        'Starts in %s': '%s 後開始',
        ' above ground': ' 地面以上',
    },
}

DISTANCE_RINGS = {
    'bg': ('Пръстени за разстояние', 'Пръстени за разстояние вкл.', 'Пръстени за разстояние изк.',
           'Показване на пръстени за разстояние около летателния апарат на картата.'),
    'ca': ('Anells de distància', 'Anells de distància activats', 'Anells de distància desactivats',
           'Mostra anells de distància al voltant de l\'aeronau al mapa.'),
    'cs': ('Kruhy vzdálenosti', 'Kruhy vzdálenosti zapnuty', 'Kruhy vzdálenosti vypnuty',
           'Zobrazit kruhy vzdálenosti kolem letadla na mapě.'),
    'da': ('Afstandsringe', 'Afstandsringe til', 'Afstandsringe fra',
           'Vis afstandsringe omkring flyet på kortet.'),
    'de': ('Entfernungsringe', 'Entfernungsringe ein', 'Entfernungsringe aus',
           'Entfernungsringe um das Flugzeug auf der Karte anzeigen.'),
    'el': ('Δακτύλιοι απόστασης', 'Δακτύλιοι απόστασης ενεργοί', 'Δακτύλιοι απόστασης ανενεργοί',
           'Εμφάνιση δακτυλίων απόστασης γύρω από το αεροσκάφος στον χάρτη.'),
    'es': ('Anillos de distancia', 'Anillos de distancia activados', 'Anillos de distancia desactivados',
           'Mostrar anillos de distancia alrededor de la aeronave en el mapa.'),
    'fi': ('Etäisyysympyrät', 'Etäisyysympyrät päällä', 'Etäisyysympyrät pois',
           'Näytä etäisyysympyrät lentokoneen ympärillä kartalla.'),
    'fr': ('Anneaux de distance', 'Anneaux de distance activés', 'Anneaux de distance désactivés',
           'Afficher les anneaux de distance autour de l\'aéronef sur la carte.'),
    'he': ('טבעות מרחק', 'טבעות מרחק פועלות', 'טבעות מרחק כבויות',
           'הצג טבעות מרחק סביב המטוס על המפה.'),
    'hr': ('Prstenovi udaljenosti', 'Prstenovi udaljenosti uklj.', 'Prstenovi udaljenosti isklj.',
           'Prikaži prstenove udaljenosti oko zrakoplova na karti.'),
    'hu': ('Távolsági gyűrűk', 'Távolsági gyűrűk be', 'Távolsági gyűrűk ki',
           'Távolsági gyűrűk megjelenítése a repülőgép körül a térképen.'),
    'it': ('Anelli di distanza', 'Anelli di distanza attivi', 'Anelli di distanza disattivi',
           'Mostra anelli di distanza intorno all\'aeromobile sulla mappa.'),
    'ja': ('距離リング', '距離リングオン', '距離リングオフ',
           '地図上の機体の周囲に距離リングを表示する。'),
    'ko': ('거리 링', '거리 링 켜짐', '거리 링 꺼짐',
           '지도에서 항공기 주위에 거리 링을 표시합니다.'),
    'lt': ('Atstumo žiedai', 'Atstumo žiedai įj.', 'Atstumo žiedai išj.',
           'Rodyti atstumo žiedus aplink orlaivį žemėlapyje.'),
    'nb': ('Avstandsringer', 'Avstandsringer på', 'Avstandsringer av',
           'Vis avstandsringer rundt flyet på kartet.'),
    'nl': ('Afstandsringen', 'Afstandsringen aan', 'Afstandsringen uit',
           'Afstandsringen rond het vliegtuig op de kaart weergeven.'),
    'pl': ('Pierścienie odległości', 'Pierścienie odległości włączone', 'Pierścienie odległości wyłączone',
           'Wyświetl pierścienie odległości wokół statku powietrznego na mapie.'),
    'pt': ('Anéis de distância', 'Anéis de distância ligados', 'Anéis de distância desligados',
           'Mostrar anéis de distância em torno da aeronave no mapa.'),
    'pt_BR': ('Anéis de distância', 'Anéis de distância ativados', 'Anéis de distância desativados',
              'Exibir anéis de distância ao redor da aeronave no mapa.'),
    'ro': ('Inele de distanță', 'Inele de distanță activate', 'Inele de distanță dezactivate',
           'Afișează inele de distanță în jurul aeronavei pe hartă.'),
    'ru': ('Кольца дальности', 'Кольца дальности вкл.', 'Кольца дальности выкл.',
           'Показывать кольца дальности вокруг самолёта на карте.'),
    'sk': ('Kruhy vzdialenosti', 'Kruhy vzdialenosti zapnuté', 'Kruhy vzdialenosti vypnuté',
           'Zobraziť kruhy vzdialenosti okolo lietadla na mape.'),
    'sl': ('Obroči razdalje', 'Obroči razdalje vkl.', 'Obroči razdalje izkl.',
           'Prikaži obroče razdalje okoli letala na zemljevidu.'),
    'sr': ('Prstenovi rastojanja', 'Prstenovi rastojanja uklj.', 'Prstenovi rastojanja isklj.',
           'Prikaži prstenove rastojanja oko vazduhoplova na mapi.'),
    'sv': ('Avståndsringar', 'Avståndsringar på', 'Avståndsringar av',
           'Visa avståndsringar runt flygplanet på kartan.'),
    'te': ('దూర వలయాలు', 'దూర వలయాలు ఆన్', 'దూర వలయాలు ఆఫ్',
           'మ్యాప్‌పై విమానం చుట్టూ దూర వలయాలను చూపించు.'),
    'tr': ('Mesafe halkaları', 'Mesafe halkaları açık', 'Mesafe halkaları kapalı',
           'Haritada uçak etrafında mesafe halkalarını göster.'),
    'uk': ('Кільця відстані', 'Кільця відстані увімк.', 'Кільця відстані вимк.',
           'Показувати кільця відстані навколо літака на карті.'),
    'vi': ('Vòng khoảng cách', 'Vòng khoảng cách bật', 'Vòng khoảng cách tắt',
           'Hiển thị vòng khoảng cách quanh máy bay trên bản đồ.'),
    'zh_CN': ('距离环', '距离环开', '距离环关', '在地图上显示飞机周围的距离环。'),
    'zh_Hant': ('距離環', '距離環開', '距離環關', '在地圖上顯示飛機周圍的距離環。'),
}

RING_KEYS = [
    'Distance rings',
    'Distance rings on',
    'Distance rings off',
    'Display distance rings around the aircraft on the map.',
]

WEGLIDE_LOAD = {
    'de': 'WeGlide-Flugzeugliste konnte nicht geladen werden.',
    'fr': 'Impossible de charger la liste des aéronefs WeGlide.',
    'es': 'No se pudo cargar la lista de aeronaves de WeGlide.',
    'nl': 'Kan de WeGlide-vliegtuiglijst niet laden.',
    'pl': 'Nie można załadować listy samolotów WeGlide.',
    'it': 'Impossibile caricare l\'elenco degli aerei WeGlide.',
    'cs': 'Nepodařilo se načíst seznam letadel WeGlide.',
    'da': 'Kunne ikke indlæse WeGlide-flylisten.',
    'nb': 'Kunne ikke laste WeGlide-flylisten.',
    'sv': 'Det gick inte att ladda WeGlide-flygplanslistan.',
    'ru': 'Не удалось загрузить список самолетов WeGlide.',
    'uk': 'Не вдалося завантажити список літаків WeGlide.',
    'ja': 'WeGlide 航空機リストをロードできませんでした。',
    'ko': 'WeGlide 항공기 목록을 로드할 수 없습니다.',
    'zh_CN': '无法加载 WeGlide 飞机列表。',
    'zh_Hant': '無法載入 WeGlide 飛機列表。',
    'bg': 'Списъкът със самолети от WeGlide не можа да бъде зареден.',
    'ca': 'No s\'ha pogut carregar la llista d\'aeronaus de WeGlide.',
    'pt': 'Não foi possível carregar a lista de aeronaves WeGlide.',
    'pt_BR': 'Não foi possível carregar a lista de aeronaves WeGlide.',
    'ro': 'Nu s-a putut încărca lista de avioane WeGlide.',
    'hu': 'Nem sikerült betölteni a WeGlide repülőgépek listáját.',
    'hr': 'Nije moguće učitati WeGlide popis zrakoplova.',
    'sl': 'Ni bilo mogoče naložiti seznama letal WeGlide.',
    'sk': 'Nepodarilo sa načítať zoznam lietadiel WeGlide.',
    'sr': 'Није могуће учитати листу WeGlide авиона.',
    'tr': 'WeGlide uçak listesi yüklenemedi.',
    'vi': 'Không thể tải danh sách máy bay WeGlide.',
    'el': 'Δεν ήταν δυνατή η φόρτωση της λίστας αεροσκαφών WeGlide.',
    'fi': 'WeGlide-lentokonelistan lataus epäonnistui.',
    'he': 'לא ניתן לטעון את רשימת מטוסי WeGlide.',
    'lt': 'Nepavyko įkelti „WeGlide“ orlaivių sąrašo.',
    'te': 'WeGlide విమానాల జాబితాను లోడ్ చేయడం సాధ్యపడలేదు.',
}

STATION_PROMPT = {
    'de': 'Möchtest du die Station %s entfernen?',
    'fr': 'Supprimer la station %s ?',
    'es': '¿Desea eliminar la estación %s?',
    'nl': 'Station %s verwijderen?',
    'pl': 'Czy usunąć stację %s?',
    'it': 'Rimuovere la stazione %s?',
    'cs': 'Chcete odstranit stanici %s?',
    'ru': 'Удалить станцию %s?',
    'uk': 'Видалити станцію %s?',
    'ja': 'ステーション %s を削除しますか？',
    'zh_CN': '是否移除站点 %s？',
    'zh_Hant': '是否移除站點 %s？',
}

MINUTES_AGO = {
    'bg': 'преди %u минути', 'ca': 'fa %u minuts', 'cs': 'před %u minutami',
    'da': 'for %u minutter siden', 'de': 'vor %u Minuten',
    'el': 'πριν από %u λεπτά', 'es': 'Hace %u minutos',
    'fi': '%u minuuttia sitten', 'fr': 'il y a %u minutes',
    'he': 'לפני %u דקות', 'hr': 'prije %u minuta',
    'hu': '%u perccel ezelőtt', 'it': '%u minuti fa',
    'ja': '%u 分前', 'ko': '%u분 전', 'lt': 'prieš %u min.',
    'nb': 'for %u minutter siden', 'nl': '%u minuten geleden',
    'pl': '%u min temu', 'pt': 'Há %u minutos',
    'pt_BR': 'Há %u minutos', 'ro': 'acum %u minute',
    'ru': '%u минут назад', 'sk': 'pred %u minútami',
    'sl': 'pred %u minutami', 'sr': 'pre %u minuta',
    'sv': 'för %u minuter sedan', 'te': '%u నిమిషాల క్రితం',
    'tr': '%u dakika önce', 'uk': '%u хвилин тому',
    'vi': '%u phút trước', 'zh_CN': '%u 分钟前',
    'zh_Hant': '%u 分鐘前',
}

LANG_UI: dict[str, dict[str, str]] = {
    'de': DE_UI,
    'fr': FR_UI,
    'es': ES_UI,
    'nl': NL_UI,
    'pl': PL_UI,
    'it': IT_UI,
    'pt': PT_UI,
    'pt_BR': PT_BR_UI,
    'ru': RU_UI,
    'uk': UK_UI,
    'ja': JA_UI,
    'ko': KO_UI,
    'zh_CN': ZH_CN_UI,
    'zh_Hant': ZH_HANT_UI,
    'cs': {**CS_UI, **CS_X},
    'da': {**DA_UI, **DA_X},
    'nb': {**NB_UI, **NB_X},
    'sv': {**SV_UI, **SV_X},
    'fi': {**FI_UI, **FI_X},
    'sk': {**SK_UI, **SK_X},
    'sl': {**SL_UI, **SL_X},
    'hr': {**HR_UI, **HR_X},
    'sr': {**SR_UI, **SR_X},
    'bg': {**BG_UI, **BG_X},
    'ro': {**RO_UI, **RO_X},
    'hu': {**HU_UI, **HU_X},
    'lt': {**LT_UI, **LT_X},
    'ca': {**CA_UI, **CA_X},
    'el': {**EL_UI, **EL_X},
    'tr': {**TR_UI, **TR_X},
    'vi': {**VI_UI, **VI_X},
    'he': {**HE_UI, **HE_X},
    'te': {**TE_UI, **TE_X},
}

# nordic_extra also has it/pt/ru/uk - merge extras for those
for lang, extra in (
    ('it', IT_X), ('pt', PT_X), ('pt_BR', PT_BR_X), ('ru', RU_X), ('uk', UK_X),
):
    LANG_UI[lang] = {**LANG_UI[lang], **extra}


def merge_dict(*dicts: dict[str, str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for d in dicts:
        out.update(d)
    return out


def fill_source_from_wind(po: polib.POFile) -> int:
    wind_tr = None
    for e in po:
        if e.msgctxt == 'Wind source' and e.msgid == 'Source' and e.msgstr:
            wind_tr = e.msgstr
            break
    if not wind_tr:
        return 0
    filled = 0
    for e in po:
        if e.msgctxt is None and e.msgid == 'Source' and not e.msgstr:
            occs = [o[0] for o in e.occurrences]
            if any('BackupRestorePanel' in o or 'ImportDataPanel' in o for o in occs):
                e.msgstr = wind_tr
                filled += 1
    return filled


def apply_dict(po: polib.POFile, table: dict[str, str]) -> int:
    filled = 0
    for e in po:
        if e.msgid not in table:
            continue
        tr = table[e.msgid]
        if not e.msgstr:
            e.msgstr = tr
            filled += 1
        elif e.msgstr == e.msgid and tr != e.msgid:
            e.msgstr = tr
            filled += 1
    return filled


def build_table(lang: str) -> dict[str, str]:
    table = dict(SHARED.get(lang, {}))
    if lang in LANG_UI:
        table.update(LANG_UI[lang])
    if lang in DISTANCE_RINGS:
        table.update(dict(zip(RING_KEYS, DISTANCE_RINGS[lang])))
    if lang in WEGLIDE_LOAD:
        table['Could not load WeGlide aircraft list.'] = WEGLIDE_LOAD[lang]
    if lang in STATION_PROMPT:
        table['Do you want to remove station %s?'] = STATION_PROMPT[lang]
    if lang in MINUTES_AGO:
        table['%u minutes ago'] = MINUTES_AGO[lang]
    if lang in COMMON_SIMPLE:
        table.update(COMMON_SIMPLE[lang])
    if lang in LONG_HELP:
        table.update(LONG_HELP[lang])
    return table


def save_po_low_churn(path: str, po: polib.POFile) -> None:
    """Write PO changes without reformatting unchanged entry blocks."""
    with tempfile.NamedTemporaryFile(
        mode='w', suffix='.po', delete=False, encoding='utf-8'
    ) as tmp:
        normalized_path = tmp.name
    try:
        po.save(normalized_path)
        subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / 'update_po_low_churn.py'),
                '--original',
                path,
                '--normalized',
                normalized_path,
            ],
            check=True,
        )
    finally:
        os.unlink(normalized_path)


def main() -> None:
    total = 0
    for path in sorted(glob.glob('po/*.po')):
        lang = path.replace('po/', '').replace('.po', '')
        po = polib.pofile(path)
        n = fill_source_from_wind(po)
        n += apply_dict(po, build_table(lang))
        if n:
            save_po_low_churn(path, po)
            print(f'{lang}: {n} translations applied')
            total += n
    print(f'Total: {total}')


if __name__ == '__main__':
    main()
