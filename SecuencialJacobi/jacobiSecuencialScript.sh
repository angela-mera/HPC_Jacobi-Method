#!/bin/bash

output_file="resultadoJacobiSecuencial.csv"

# Comprueba si el archivo ya existe y, si no, añade los encabezados
if [ ! -f "$output_file" ]; then
  echo "Longitud,Tiempo" > "$output_file"
fi

for i in 5000 25000 50000 100000 150000 200000 250000; do
  echo "Ejecutando script para arreglo de longitud: $i"

  for j in {1..10}; do
    ./jacobiSecuencialExe $i $i >> "$output_file"
  done
    
  echo "" >> "$output_file"
done

echo "Pruebas finalizadas."
