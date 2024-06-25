/*
 * matriz_keyboard.h
 *
 *  Created on: 17 de Abril 2024
 *      Author: German Velardez
 *      Email: germanvelardez16@gmail.com
 */




#ifndef _KEYBOARD_H

/**
 * @brief Init the keyboard
 * 
 */
void keyboard_init();


/**
 * @brief Get the last keyboard, you need to
 *  get 1 in keyboard_check() before to get a valid char 
 * 
 * @return char return the last_char
 */
char keyboard_get_char();


/**
 * @brief Check the keyboard. 
 * if its pressed any key,save the char.
 * 
 * @return int  its 1 if key pressed or 0 if not
 */
int keyboard_check();





#endif
