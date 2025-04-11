import React, { useState, useEffect } from 'react';
import { 
  Text, 
  View, 
  TouchableOpacity, 
  Alert, 
  StyleSheet, 
  Image, 
  ImageBackground 
} from 'react-native';
import * as Font from 'expo-font';

export default function StartMenu() {
  const [fontsLoaded, setFontsLoaded] = useState(false);

  useEffect(() => {
    async function loadFonts() {
      await Font.loadAsync({
        "Koulen-Regular": require("./assets/fonts/Koulen-Regular.ttf"),
      });
      setFontsLoaded(true);
    }
    loadFonts();
  }, []);

  if (!fontsLoaded) {
    return <View style={styles.loadingContainer}><Text>Loading...</Text></View>;
  }

  return (
    <ImageBackground
      source={require("./assets/images/Background.png")}
      style={styles.background}
    >
      <View style={styles.container}>
        {/* Left Side - Logo and Play Button */}
        <View style={styles.leftContainer}>
          <Image 
            source={require("./assets/images/logo.png")} 
            style={styles.logo} 
          />
          <TouchableOpacity
            style={styles.button}
            onPress={() => Alert.alert('Button pressed')}
          >
            <Text style={styles.buttonText}>PLAY</Text>
          </TouchableOpacity>
        </View>

        {/* Right Side - Promo Image */}
        <View style={styles.rightContainer}>
          <Image 
            source={require("./assets/images/promo-image.png")} 
            style={styles.promoImage} 
          />
        </View>
      </View>
    </ImageBackground>
  );
}

const styles = StyleSheet.create({
  background: {
    flex: 1,
    resizeMode: "cover",
    justifyContent: "center",
    alignItems: "center",
  },
  container: {
    flex: 1,
    flexDirection: 'row', // Split the layout into two sections
    width: '100%',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 50,
  },
  leftContainer: {
    alignItems: 'center',
    flex: 1,
  },
  rightContainer: {
    flex: 1,
    alignItems: 'center',
  },
  logo: {
    width: 400,
    height: 250,
    marginBottom: 10,
    marginTop: -100
  },
  button: {
    backgroundColor: '#EE4266',
    paddingHorizontal: 40,
    borderRadius: 100,
    width: 300,
    height: 100,
    justifyContent: 'center',
    alignItems: 'center'
  },
  buttonText: {
    color: 'white',
    fontFamily: 'Koulen-Regular',
    fontSize: 45,
  },
  promoImage: {
    width: 500,
    height: 500,
    resizeMode: 'contain',
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
});
