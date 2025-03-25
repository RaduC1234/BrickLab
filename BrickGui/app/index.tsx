import React from 'react';
import { Text, View, TouchableOpacity, Alert, StyleSheet } from 'react-native';

export default function Index() {
  return (
    <View style={styles.container}>
      <Text style={styles.text}>Hielau</Text>
      <TouchableOpacity
        style={styles.button}
        onPress={() => Alert.alert('Button pressed')}
      >
        <Text style={styles.buttonText}>Press me</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: 'orange',
  },
  text: {
    color: 'white',
    fontSize: 50,
    marginBottom: 20,
  },
  button: {
    backgroundColor: 'purple',
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 20, // Adjust the value to control the roundness
  },
  buttonText: {
    color: 'white',
    fontSize: 16,
  },
});
