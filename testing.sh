#!/bin/bash
# ============================================================================
# Codexion - Batería de autocomprobación para defensa / evaluación
# ============================================================================
# Uso: colócalo en la raíz del repo (donde está el Makefile) y ejecuta:
#   chmod +x self_check.sh
#   ./self_check.sh
#
# Requiere: norminette (pip install norminette --break-system-packages),
# valgrind (apt install valgrind), y opcionalmente gcc/cc con soporte TSAN.
# Si alguna herramienta no está instalada, ese bloque se salta con un aviso.
# ============================================================================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

ok()   { echo -e "${GREEN}[OK]${NC} $1"; PASS=$((PASS+1)); }
bad()  { echo -e "${RED}[FAIL]${NC} $1"; FAIL=$((FAIL+1)); }
warn() { echo -e "${YELLOW}[AVISO]${NC} $1"; }
hdr()  { echo ""; echo "==================== $1 ===================="; }

# ----------------------------------------------------------------------------
hdr "1. COMPILACION"
# ----------------------------------------------------------------------------
make fclean > /dev/null 2>&1
if make > /tmp/codexion_build.log 2>&1; then
	if grep -qi "warning:" /tmp/codexion_build.log; then
		bad "compila pero hay warnings en la salida (revisa /tmp/codexion_build.log)"
	else
		ok "compila limpio con -Wall -Wextra -Werror -pthread"
	fi
else
	bad "NO COMPILA -- revisa /tmp/codexion_build.log"
	cat /tmp/codexion_build.log
fi

if [ -f codexion ]; then
	ok "binario 'codexion' generado"
else
	bad "binario 'codexion' no encontrado, abortando resto de tests"
	exit 1
fi

# Comprueba que no hace relink innecesario
touch coders.c 2>/dev/null
RELINK_OUT=$(make 2>&1)
RELINK_COUNT=$(echo "$RELINK_OUT" | grep -c "\-c .*\.c")
if [ "$RELINK_COUNT" -le 1 ]; then
	ok "make no recompila todo de cero al tocar un solo archivo (no hace relink innecesario)"
else
	warn "al tocar un archivo se recompilaron $RELINK_COUNT ficheros -- revisa dependencias del Makefile"
fi

# ----------------------------------------------------------------------------
hdr "2. NORMINETTE"
# ----------------------------------------------------------------------------
if command -v norminette > /dev/null 2>&1; then
	NORM_OUT=$(norminette *.c *.h 2>&1)
	if echo "$NORM_OUT" | grep -q "Error!"; then
		bad "norminette encontró errores:"
		echo "$NORM_OUT" | grep -A20 "Error!"
	else
		ok "norminette pasa limpio en todos los .c/.h"
	fi
else
	warn "norminette no instalado -- instala con: pip install norminette --break-system-packages"
fi

# ----------------------------------------------------------------------------
hdr "3. VALIDACION DE ARGUMENTOS"
# ----------------------------------------------------------------------------
check_rejects() {
	./codexion $1 > /tmp/argtest.log 2>&1
	CODE=$?
	if [ "$CODE" -ne 0 ]; then
		ok "rechaza correctamente: '$1' (exit $CODE)"
	else
		bad "DEBERIA rechazar pero no lo hizo: '$1'"
	fi
}

check_rejects "-3 800 200 200 200 5 0 fifo"
check_rejects "abc 800 200 200 200 5 0 fifo"
check_rejects "3 800 200 200 200 5 0 round_robin"
check_rejects "0 800 200 200 200 5 0 fifo"
check_rejects "3 800 200"
check_rejects ""

# caso válido: dongle_cooldown = 0 debe ser aceptado
./codexion 2 800 200 200 200 1 0 fifo > /tmp/argtest_valid.log 2>&1 &
PID=$!
sleep 0.3
if kill -0 $PID 2>/dev/null; then
	ok "dongle_cooldown=0 es aceptado y arranca la simulación"
	wait $PID
else
	wait $PID
	CODE=$?
	if [ "$CODE" -eq 0 ]; then
		ok "dongle_cooldown=0 es aceptado y arranca la simulación"
	else
		bad "dongle_cooldown=0 (válido) fue rechazado"
	fi
fi

# ----------------------------------------------------------------------------
hdr "4. CICLO COMPLETO -- numero exacto de compilaciones (bug de over-compile)"
# ----------------------------------------------------------------------------
echo "Ejecutando 5 veces: ./codexion 3 800 200 200 200 5 0 fifo"
echo "Cada coder debe compilar EXACTAMENTE 5 veces, ni una más."
ALL_OK=1
for i in 1 2 3 4 5; do
	./codexion 3 800 200 200 200 5 0 fifo > /tmp/cc_$i.log 2>&1
	COUNTS=$(grep "is compiling" /tmp/cc_$i.log | awk '{print $2}' | sort | uniq -c | awk '{print $1}')
	BAD_RUN=0
	for c in $COUNTS; do
		if [ "$c" -ne 5 ]; then
			BAD_RUN=1
		fi
	done
	if [ "$BAD_RUN" -eq 1 ]; then
		bad "run $i: alguien compiló un número distinto de 5 veces -- $(grep 'is compiling' /tmp/cc_$i.log | awk '{print $2}' | sort | uniq -c | tr '\n' ' ')"
		ALL_OK=0
	fi
done
if [ "$ALL_OK" -eq 1 ]; then
	ok "5/5 ejecuciones: cada coder compiló exactamente 5 veces"
fi

# ----------------------------------------------------------------------------
hdr "5. TERMINACION LIMPIA Y TIMING"
# ----------------------------------------------------------------------------
START=$(date +%s%N)
timeout 10 ./codexion 3 800 200 200 200 5 0 fifo > /tmp/timing.log 2>&1
CODE=$?
END=$(date +%s%N)
ELAPSED_MS=$(( (END - START) / 1000000 ))
if [ "$CODE" -eq 0 ] && [ "$ELAPSED_MS" -lt 9000 ]; then
	ok "el programa termina por sí solo en ${ELAPSED_MS}ms (exit 0, sin necesitar timeout)"
elif [ "$CODE" -eq 124 ]; then
	bad "el programa NO terminó solo, tuvo que matarlo timeout (posible cuelgue)"
else
	warn "terminó con exit $CODE en ${ELAPSED_MS}ms -- revisa manualmente"
fi

# ----------------------------------------------------------------------------
hdr "6. BURNOUT -- precision de los 10ms exigidos por el subject"
# ----------------------------------------------------------------------------
echo "Ejecutando 5 veces: ./codexion 3 200 500 200 200 5 0 fifo (time_to_compile > time_to_burnout)"
echo "Se mide el timestamp INTERNO que imprime el propio programa, no el tiempo total del proceso"
echo "(arrancar varios hilos añade overhead del sistema operativo que no cuenta como parte del agotamiento)."
ALL_OK=1
for i in 1 2 3 4 5; do
	./codexion 3 200 500 200 200 5 0 fifo > /tmp/burnout_$i.log 2>&1
	LAST_LINE=$(tail -1 /tmp/burnout_$i.log)
	if ! echo "$LAST_LINE" | grep -q "burned out"; then
		bad "run $i: no se detectó burned out -- $LAST_LINE"
		ALL_OK=0
		continue
	fi
	INTERNAL_MS=$(echo "$LAST_LINE" | awk '{print $1}')
	# time_to_burnout=200 en este test; tolerancia generosa de 30ms
	DIFF=$((INTERNAL_MS - 200))
	if [ "$DIFF" -lt 0 ] || [ "$DIFF" -gt 30 ]; then
		bad "run $i: timestamp interno de burnout = ${INTERNAL_MS}ms (esperado ~200ms, diff=${DIFF}ms)"
		ALL_OK=0
	fi
done
if [ "$ALL_OK" -eq 1 ]; then
	ok "5/5 ejecuciones: timestamp interno de burnout dentro de margen razonable (~200ms +/- 30ms)"
fi

# ----------------------------------------------------------------------------
hdr "7. CASO BORDE -- un solo coder"
# ----------------------------------------------------------------------------
OUT=$(./codexion 1 800 200 200 200 5 0 fifo 2>&1)
if echo "$OUT" | grep -q "burned out" && [ "$(echo "$OUT" | wc -l)" -eq 1 ]; then
	ok "1 coder: burnout limpio, sin deadlock ni crash -- '$OUT'"
else
	bad "1 coder: comportamiento inesperado -- '$OUT'"
fi

# ----------------------------------------------------------------------------
hdr "8. EDF -- no debe agotar a nadie con parametros viables"
# ----------------------------------------------------------------------------
echo "Ejecutando 10 veces: ./codexion 5 2000 200 100 100 5 50 edf"
ALL_OK=1
for i in $(seq 1 10); do
	./codexion 5 2000 200 100 100 5 50 edf > /tmp/edf_$i.log 2>&1
	if grep -q "burned out" /tmp/edf_$i.log; then
		bad "run $i: EDF produjo un burnout con parámetros holgados -- $(grep 'burned out' /tmp/edf_$i.log)"
		ALL_OK=0
	fi
done
if [ "$ALL_OK" -eq 1 ]; then
	ok "10/10 ejecuciones EDF: nadie se agotó con parámetros viables"
fi

# ----------------------------------------------------------------------------
hdr "9. ESTRES -- muchos coders, alta contencion"
# ----------------------------------------------------------------------------
ALL_OK=1
for i in 1 2 3; do
	./codexion 10 1000 100 50 50 6 30 edf > /tmp/stress_$i.log 2>&1
	COUNTS=$(grep "is compiling" /tmp/stress_$i.log | awk '{print $2}' | sort | uniq -c | awk '{print $1}')
	for c in $COUNTS; do
		if [ "$c" -ne 6 ]; then
			ALL_OK=0
		fi
	done
done
if [ "$ALL_OK" -eq 1 ]; then
	ok "3/3 ejecuciones con 10 coders: todos compilan exactamente el número requerido"
else
	bad "estrés con 10 coders: algún coder compiló un número incorrecto de veces"
fi

# ----------------------------------------------------------------------------
hdr "10. MEMORIA -- valgrind (leaks)"
# ----------------------------------------------------------------------------
if command -v valgrind > /dev/null 2>&1; then
	VALG_OUT=$(valgrind --leak-check=full --error-exitcode=42 ./codexion 3 800 200 200 200 5 0 fifo 2>&1)
	VALG_CODE=$?
	if echo "$VALG_OUT" | grep -q "All heap blocks were freed -- no leaks are possible" && [ "$VALG_CODE" -ne 42 ]; then
		ok "valgrind: 0 leaks, 0 errores de memoria"
	else
		bad "valgrind reportó problemas -- revisa la salida completa:"
		echo "$VALG_OUT" | tail -20
	fi
else
	warn "valgrind no instalado -- instala con: apt install valgrind (Linux) o brew install valgrind (no disponible en macOS moderno, usar Linux/VM)"
fi

# ----------------------------------------------------------------------------
hdr "11. CONCURRENCIA -- ThreadSanitizer (data races)"
# ----------------------------------------------------------------------------
if cc -fsanitize=thread -pthread -o /tmp/codexion_tsan *.c > /tmp/tsan_build.log 2>&1; then
	TSAN_OK=1
	for i in 1 2 3; do
		TSAN_OPTIONS="halt_on_error=0" /tmp/codexion_tsan 8 1000 100 50 50 6 30 edf > /tmp/tsan_$i.log 2>&1
		if grep -q "WARNING: ThreadSanitizer" /tmp/tsan_$i.log; then
			TSAN_OK=0
			bad "TSAN run $i: data race detectada -- revisa /tmp/tsan_$i.log"
		fi
	done
	if [ "$TSAN_OK" -eq 1 ]; then
		ok "3/3 ejecuciones bajo ThreadSanitizer: 0 data races"
	fi
	rm -f /tmp/codexion_tsan
else
	warn "no se pudo compilar con -fsanitize=thread, revisa /tmp/tsan_build.log"
fi

# ----------------------------------------------------------------------------
hdr "12. README"
# ----------------------------------------------------------------------------
if [ -f README.md ]; then
	FIRST_LINE=$(head -1 README.md)
	if echo "$FIRST_LINE" | grep -q "^\*.*42.*\*$"; then
		ok "primera línea del README en formato correcto: $FIRST_LINE"
	else
		bad "primera línea del README no parece seguir el formato exigido (cursiva + texto exacto)"
	fi
	for section in "Description" "Instructions" "Resources" "Blocking cases handled" "Thread synchronization mechanisms"; do
		if grep -q "## $section" README.md; then
			ok "sección presente: $section"
		else
			bad "FALTA sección obligatoria: $section"
		fi
	done
else
	bad "README.md no encontrado en este directorio"
fi

# ----------------------------------------------------------------------------
hdr "RESUMEN FINAL"
# ----------------------------------------------------------------------------
echo ""
echo -e "${GREEN}Pasados: $PASS${NC}   ${RED}Fallidos: $FAIL${NC}"
if [ "$FAIL" -eq 0 ]; then
	echo -e "${GREEN}Todo correcto. Recuerda confirmar a mano la estructura de carpetas del repo Git${NC}"
	echo -e "${GREEN}(Makefile, *.c, *.h dentro de coders/, según pide el subject).${NC}"
else
	echo -e "${RED}Hay $FAIL punto(s) que revisar antes de la defensa.${NC}"
fi

# limpieza de archivos objeto generados por este script
make fclean > /dev/null 2>&1